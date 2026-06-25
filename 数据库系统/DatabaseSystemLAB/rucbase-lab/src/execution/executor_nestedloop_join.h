/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once
#include <algorithm>
#include <cstring>

#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

/*
 * 验收讲解：NestedLoopJoinExecutor 实现最基础的嵌套循环连接。
 * 左子执行器 left_ 是外层循环，右子执行器 right_ 是内层循环；对每条左记录都从头扫描右记录。
 * 找到满足 join 条件的一对记录后，执行器停在这对记录上，等待上层调用 Next() 取走连接结果。
 * join 输出记录的内存布局是“左记录字节 + 右记录字节”，所以右表列的 offset 要整体加上 left_->tupleLen()。
 * 这个实现没有使用索引优化，复杂度接近左表记录数乘右表记录数，适合实验四基础查询执行。
 */

class NestedLoopJoinExecutor : public AbstractExecutor {
   private:
    std::unique_ptr<AbstractExecutor> left_;    // 左子执行器，作为嵌套循环的外层，一条左记录会搭配扫描完整个右输入。
    std::unique_ptr<AbstractExecutor> right_;   // 右子执行器，作为嵌套循环的内层，每换一条左记录都要重新 beginTuple()。
    size_t len_;                                // join 后一条输出记录的长度 = 左记录长度 + 右记录长度。
    std::vector<ColMeta> cols_;                 // join 后输出记录的所有列，先放左输入列，再放右输入列。

    std::vector<Condition> fed_conds_;          // join 条件，例如 t1.id = t2.id；为空时表示没有 join 条件，会形成笛卡尔积。
    bool isend;                                 // isend 表示 join 算子整体是否已经没有下一对匹配记录。

   public:
    NestedLoopJoinExecutor(std::unique_ptr<AbstractExecutor> left, std::unique_ptr<AbstractExecutor> right, 
                            std::vector<Condition> conds) {
        // 接管左右子执行器的所有权；之后 join 通过它们向下拉取左右两边的当前记录。
        left_ = std::move(left);
        right_ = std::move(right);

        // 输出记录直接把左右记录拼接，所以长度是两边长度相加。
        len_ = left_->tupleLen() + right_->tupleLen();
        cols_ = left_->cols();
        auto right_cols = right_->cols();
        // 右表字段原本的 offset 是相对右表记录起点计算的。
        // join 后右记录被拼在左记录后面，所以每个右表字段都要整体后移 left_->tupleLen() 字节。
        // 例如左记录长 12 字节，右表某列原 offset 是 4，则 join 后该列 offset 是 16。
        for (auto &col : right_cols) {
            col.offset += left_->tupleLen();
        }

        // cols_ 的顺序要和实际输出字节布局一致：先左列，再右列。
        cols_.insert(cols_.end(), right_cols.begin(), right_cols.end());
        isend = false;
        fed_conds_ = std::move(conds);

    }

    void beginTuple() override {
        // 嵌套循环连接的外层是 left_，内层是 right_。
        // 初始化时先把左、右子执行器都定位到第一条记录，再寻找第一对满足连接条件的记录。
        left_->beginTuple();
        if (left_->is_end()) {
            // 左输入为空时，不可能产生任何 join 结果。
            isend = true;
            return;
        }
        right_->beginTuple();
        isend = false;
        find_next_match();
    }

    void nextTuple() override {
        if (isend) {
            return;
        }

        // 当前组合已经作为结果返回过，因此先移动右表到下一条，再继续寻找下一对匹配记录。
        // 如果右表后面没有匹配，find_next_match 会自动推动左表并重置右表。
        right_->nextTuple();
        find_next_match();
    }

    std::unique_ptr<RmRecord> Next() override {
        if (is_end()) {
            return nullptr;
        }

        // 当前 left_ 和 right_ 各自指向一条满足连接条件的记录。
        // join 输出就是把左记录的数据放前面，右记录的数据接在后面。
        auto left_rec = left_->Next();
        auto right_rec = right_->Next();
        auto joined = std::make_unique<RmRecord>(len_);
        memcpy(joined->data, left_rec->data, left_->tupleLen());
        memcpy(joined->data + left_->tupleLen(), right_rec->data, right_->tupleLen());
        return joined;
    }

    // join 输出是临时组合记录，不对应某张表里的单个 Rid，因此返回抽象 rid 占位。
    Rid &rid() override { return _abstract_rid; }

    bool is_end() const override { return isend; }
    
    std::string getType() override { return "NestedLoopJoinExecutor"; }

    size_t tupleLen() const override { return len_; }

    const std::vector<ColMeta> &cols() const override { return cols_; }

    private:
    void find_next_match() {
        // 目标：让 left_ 和 right_ 停在下一对满足 fed_conds_ 的记录上。
        // 对每一条左记录，遍历所有右记录；右表扫完后，左表前进一条，右表重新 begin。
        while (!left_->is_end()) {
            while (!right_->is_end()) {
                auto left_rec = left_->Next();
                auto right_rec = right_->Next();
                // eval_conds 为空时返回 true，因此没有连接条件时会返回所有左右组合，也就是笛卡尔积。
                if (eval_conds(left_rec.get(), right_rec.get(), fed_conds_, cols_)) {
                    return;
                }
                right_->nextTuple();
            }
            // 当前左记录已经和所有右记录尝试完毕，换下一条左记录。
            left_->nextTuple();
            if (left_->is_end()) {
                break;
            }
            // 每换一条左记录，右输入都要重新从第一条记录开始匹配。
            right_->beginTuple();
        }
        // 左输入也扫描结束，说明所有组合都检查完了。
        isend = true;
    }

    // value_ptr 用连接后列元数据的 offset 判断字段来自左记录还是右记录。
    // offset 小于左记录长度，说明字段在左半部分；否则字段在右半部分，需要减去左记录长度再访问右记录。
    const char *value_ptr(const RmRecord *lhs_rec, const RmRecord *rhs_rec, const ColMeta &col) const {
        if (col.offset < static_cast<int>(left_->tupleLen())) {
            return lhs_rec->data + col.offset;
        }
        return rhs_rec->data + (col.offset - left_->tupleLen());
    }

    // 比较两个字段的原始字节。逻辑和 SeqScanExecutor 中的比较函数一致。
    int compare_value(const char *lhs, const char *rhs, ColType type, int len) const {
        if (type == TYPE_INT) {
            int lhs_val = *reinterpret_cast<const int *>(lhs);
            int rhs_val = *reinterpret_cast<const int *>(rhs);
            return (lhs_val > rhs_val) - (lhs_val < rhs_val);
        }
        if (type == TYPE_FLOAT) {
            float lhs_val = *reinterpret_cast<const float *>(lhs);
            float rhs_val = *reinterpret_cast<const float *>(rhs);
            return (lhs_val > rhs_val) - (lhs_val < rhs_val);
        }
        return memcmp(lhs, rhs, len);
    }

    // 把 compare_value 的三态结果翻译成 SQL 比较符的布尔结果。
    bool compare_result(int cmp, CompOp op) const {
        switch (op) {
            case OP_EQ: return cmp == 0;
            case OP_NE: return cmp != 0;
            case OP_LT: return cmp < 0;
            case OP_GT: return cmp > 0;
            case OP_LE: return cmp <= 0;
            case OP_GE: return cmp >= 0;
        }
        return false;
    }

    bool eval_cond(const RmRecord *lhs_rec, const RmRecord *rhs_rec, const Condition &cond, const std::vector<ColMeta> &rec_cols) {
        // rec_cols 是连接后记录的字段元数据，其中右表字段 offset 已经整体加上 left_->tupleLen()。
        // 因此先用 get_col 找到条件中的列，再用 value_ptr 判断它实际在左记录还是右记录。
        auto lhs_col = get_col(rec_cols, cond.lhs_col);
        const char *lhs = value_ptr(lhs_rec, rhs_rec, *lhs_col);

        const char *rhs = nullptr;
        int rhs_len = lhs_col->len;
        if (cond.is_rhs_val) {
            // join 条件右侧也可能是常量，例如 t1.id = 1。
            rhs = cond.rhs_val.raw->data;
        } else {
            // 更常见的是列和列比较，例如 t1.id = t2.id。
            auto rhs_col = get_col(rec_cols, cond.rhs_col);
            rhs = value_ptr(lhs_rec, rhs_rec, *rhs_col);
            rhs_len = rhs_col->len;
        }

        int cmp = compare_value(lhs, rhs, lhs_col->type, std::min(lhs_col->len, rhs_len));
        return compare_result(cmp, cond.op);
    }

    // 多个 join 条件之间是 AND 关系，必须全部满足才返回当前左右记录组合。
    bool eval_conds(const RmRecord *lhs_rec, const RmRecord *rhs_rec, const std::vector<Condition> &conds, const std::vector<ColMeta> &rec_cols) {
        return std::all_of(conds.begin(), conds.end(),
            [&](const Condition &cond) { return eval_cond(lhs_rec, rhs_rec, cond, rec_cols); }
        );
    }
};




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
 * 验收讲解：SeqScanExecutor 是查询执行树的叶子节点，负责从一张表中顺序产生满足 where 条件的记录。
 * 执行器采用 iterator/Volcano 模型：beginTuple() 定位第一条结果，nextTuple() 移动到下一条结果，Next() 返回当前结果。
 * beginTuple/nextTuple 只定位当前满足条件的 rid_，Next() 才按 rid_ 读取记录内容；这样上层算子可以统一“拉取”结果。
 * 条件比较时，列值来自记录 data + offset，常量值来自 Analyze 阶段准备好的 raw 字节。
 */

class SeqScanExecutor : public AbstractExecutor {
   private:
    std::string tab_name_;              // 被顺序扫描的表名，例如 select * from student 中的 student。
    std::vector<Condition> conds_;      // Analyze 阶段传进来的 where 条件，记录了左列、比较符、右值/右列。
    RmFileHandle *fh_;                  // 表的数据文件句柄；真正读记录时调用 fh_->get_record(rid, context)。
    std::vector<ColMeta> cols_;         // 本算子输出记录的列布局；顺序扫描输出整张表，所以就是表的全部列。
    size_t len_;                        // 一条输出记录的字节长度，等于最后一列 offset + 最后一列 len。
    std::vector<Condition> fed_conds_;  // 实际用于过滤记录的条件。本实验中它直接等于 conds_，保留这个变量是为了贴合执行器框架。

    Rid rid_;                           // 当前满足 where 条件的记录位置；Next() 会按照这个 rid_ 取记录。
    std::unique_ptr<RecScan> scan_;     // Lab2 的表扫描迭代器，负责按页号和槽号遍历所有有效记录。

    SmManager *sm_manager_;             // 系统管理器，用来取得表元数据 db_ 和已经打开的表文件句柄 fhs_。

   public:
    SeqScanExecutor(SmManager *sm_manager, std::string tab_name, std::vector<Condition> conds, Context *context) {
        // 构造函数只准备“怎么扫描”，不会马上开始扫描；真正扫描从 beginTuple() 开始。
        sm_manager_ = sm_manager;
        tab_name_ = std::move(tab_name);
        conds_ = std::move(conds);

        // TabMeta 保存表结构，包括所有列的类型、长度、offset 等信息。
        // 后续判断 where 条件和返回结果，都需要依赖这些列元数据解释记录字节。
        TabMeta &tab = sm_manager_->db_.get_table(tab_name_);

        // fhs_ 是实验三 open_db/create_table 中维护的“表名 -> 表文件句柄”映射。
        // at(tab_name_) 如果找不到表会抛异常，这也能暴露执行计划中表名错误的问题。
        fh_ = sm_manager_->fhs_.at(tab_name_).get();
        cols_ = tab.cols;
        len_ = cols_.back().offset + cols_.back().len;

        // context_ 中保存事务、锁管理、日志等运行时信息；当前实验主要把它继续传给记录管理接口。
        context_ = context;

        fed_conds_ = conds_;
    }

    /**
     * @brief 构建表迭代器scan_,并开始迭代扫描,直到扫描到第一个满足谓词条件的元组停止,并赋值给rid_
     *
     */
    void beginTuple() override {
        // SeqScanExecutor 是顺序扫描算子，底层依赖 Lab2 实现的 RmScan 逐条遍历表记录。
        // RmScan 只知道“下一个有效 slot 在哪里”，不知道 SQL 的 where 条件，所以条件过滤要在这里完成。
        scan_ = std::make_unique<RmScan>(fh_);
        while (!scan_->is_end()) {
            // scan_->rid() 是当前候选记录的位置；fh_->get_record 根据 rid 把记录内容读出来。
            auto rec = fh_->get_record(scan_->rid(), context_);
            // eval_conds 返回 true，说明当前记录满足所有 where 条件，可以作为第一条结果。
            if (eval_conds(rec.get(), fed_conds_, cols_)) {
                rid_ = scan_->rid();
                return;
            }
            // 当前记录不满足条件，继续让 RmScan 移动到下一条有效记录。
            scan_->next();
        }
        // 扫完整张表都没找到结果，用 RM_NO_PAGE 标记执行器已经结束。
        rid_ = {RM_NO_PAGE, -1};
    }

    /**
     * @brief 从当前scan_指向的记录开始迭代扫描,直到扫描到第一个满足谓词条件的元组停止,并赋值给rid_
     *
     */
    void nextTuple() override {
        // 如果还没有 begin，或者底层扫描器已经结束，就直接把 rid_ 置为结束标志。
        if (scan_ == nullptr || scan_->is_end()) {
            rid_ = {RM_NO_PAGE, -1};
            return;
        }

        // 当前 rid_ 已经是上一条结果，所以先让 RmScan 前进一步，再继续找下一条满足条件的记录。
        scan_->next();
        while (!scan_->is_end()) {
            auto rec = fh_->get_record(scan_->rid(), context_);
            if (eval_conds(rec.get(), fed_conds_, cols_)) {
                rid_ = scan_->rid();
                return;
            }
            scan_->next();
        }
        // 没有下一条满足条件的记录，告诉上层算子本扫描结束。
        rid_ = {RM_NO_PAGE, -1};
    }

    /**
     * @brief 返回下一个满足扫描条件的记录
     *
     * @return std::unique_ptr<RmRecord>
     */
    std::unique_ptr<RmRecord> Next() override {
        // beginTuple/nextTuple 只负责定位 rid_，真正返回记录时再通过 RmFileHandle 读取。
        // 注意 Next() 不会移动扫描器；移动到下一条结果由上层调用 nextTuple() 完成。
        if (is_end()) {
            return nullptr;
        }
        return fh_->get_record(rid_, context_);
    }

    Rid &rid() override { return rid_; }

    // 只要还没创建扫描器、扫描器已结束、或者 rid_ 被置成 RM_NO_PAGE，都代表没有当前结果。
    bool is_end() const override { return scan_ == nullptr || scan_->is_end() || rid_.page_no == RM_NO_PAGE; }
    
    std::string getType() override { return "SeqScanExecutor"; }

    size_t tupleLen() const override { return len_; }

    const std::vector<ColMeta> &cols() const override { return cols_; }

    private:
    // compare_value 的输入都是字段在记录字节数组中的起始地址。
    // 因为底层记录统一保存为 char*，所以比较前必须按列类型把字节解释成 int、float 或定长字符串。
    int compare_value(const char *lhs, const char *rhs, ColType type, int len) const {
        if (type == TYPE_INT) {
            // reinterpret_cast 把 char* 指向的 4 个字节解释成 int，再做数值比较。
            int lhs_val = *reinterpret_cast<const int *>(lhs);
            int rhs_val = *reinterpret_cast<const int *>(rhs);
            return (lhs_val > rhs_val) - (lhs_val < rhs_val);
        }
        if (type == TYPE_FLOAT) {
            // float 同理，把字节解释成浮点数后比较大小。
            float lhs_val = *reinterpret_cast<const float *>(lhs);
            float rhs_val = *reinterpret_cast<const float *>(rhs);
            return (lhs_val > rhs_val) - (lhs_val < rhs_val);
        }
        // 字符串是定长 char 数组，直接按字节字典序比较。
        return memcmp(lhs, rhs, len);
    }

    // compare_value 只给出小于/等于/大于的结果；compare_result 再把它翻译成 SQL 比较符的真假。
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

    bool eval_cond(const RmRecord *rec, const Condition &cond, const std::vector<ColMeta> &rec_cols) {
        // 条件左边一定是列。通过列元数据中的 offset 找到该列在记录 data 中的位置。
        auto lhs_col = get_col(rec_cols, cond.lhs_col);
        const char *lhs = rec->data + lhs_col->offset;

        const char *rhs = nullptr;
        int rhs_len = lhs_col->len;
        if (cond.is_rhs_val) {
            // 右边是常量值时，Analyze 阶段已经把常量转换成 raw 字节。
            // 例如 where id = 3，rhs_val.raw 中存的就是整数 3 的二进制表示。
            rhs = cond.rhs_val.raw->data;
        } else {
            // 右边是列时，在同一条记录中根据 rhs_col 的 offset 取值。
            // 例如 where a = b，就分别取 a 列和 b 列的字节区域比较。
            auto rhs_col = get_col(rec_cols, cond.rhs_col);
            rhs = rec->data + rhs_col->offset;
            rhs_len = rhs_col->len;
        }
        // 类型检查在语义分析阶段已经完成，这里直接按左列类型比较。
        int cmp = compare_value(lhs, rhs, lhs_col->type, std::min(lhs_col->len, rhs_len));
        return compare_result(cmp, cond.op);
    }

    // eval_conds 是 where 条件的“与”逻辑：所有条件都满足，当前记录才是结果。
    // conds 为空时 all_of 返回 true，正好对应 SQL 中没有 where 条件、全表记录都满足的情况。
    bool eval_conds(const RmRecord *rec, const std::vector<Condition> &conds, const std::vector<ColMeta> &rec_cols) {
        return std::all_of(conds.begin(), conds.end(),
            [&](const Condition &cond) { return eval_cond(rec, cond, rec_cols); }
        );
    }
};



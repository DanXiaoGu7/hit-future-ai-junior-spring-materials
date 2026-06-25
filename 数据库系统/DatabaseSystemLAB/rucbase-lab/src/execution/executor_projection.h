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
#include <cstring>

#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

/*
 * 验收讲解：ProjectionExecutor 对应 select 后面的列选择。
 * 子执行器 prev_ 产生完整记录，ProjectionExecutor 只把需要输出的列拷贝到一条更短的新记录里。
 * 投影后的列 offset 必须重新从 0 累加计算，因为输出记录的布局已经不同于原表或 join 后的完整记录。
 * 例如原记录是 id,name,age，而 select name,id 的输出记录布局会变成 name 在前、id 在后。
 */

class ProjectionExecutor : public AbstractExecutor {
   private:
    std::unique_ptr<AbstractExecutor> prev_;        // 投影节点的子执行器，可能是顺序扫描，也可能是 join 的结果。
    std::vector<ColMeta> cols_;                     // 投影后输出的列元数据，offset 已经按新结果重新计算。
    size_t len_;                                    // 投影后一条结果记录的总字节长度，只包含被 select 的列。
    std::vector<size_t> sel_idxs_;                  // 每个输出列在 prev_->cols() 中的下标，用来从子记录中定位源字段。

   public:
    ProjectionExecutor(std::unique_ptr<AbstractExecutor> prev, const std::vector<TabCol> &sel_cols) {
        // 接管子执行器所有权；之后 Projection 通过 prev_ 向下拉取完整记录。
        prev_ = std::move(prev);

        size_t curr_offset = 0;
        auto &prev_cols = prev_->cols();
        // sel_idxs_ 保存的是所选列在子执行器输出列数组中的下标。
        // 之后 Next() 拿到子记录后，就能根据这些下标找到源列 offset，把对应字段拷贝到投影结果中。
        for (auto &sel_col : sel_cols) {
            // get_col 根据表名和列名，在子执行器输出列中找到对应列元数据。
            auto pos = get_col(prev_cols, sel_col);
            sel_idxs_.push_back(pos - prev_cols.begin());
            auto col = *pos;
            // curr_offset 是投影结果中的新偏移，不是原表偏移。
            // 例如 select name,id 时，name 在结果中从 0 开始，id 紧跟 name 后面。
            col.offset = curr_offset;
            curr_offset += col.len;
            cols_.push_back(col);
        }
        // 所有被选列长度加起来，就是投影结果记录的总长度。
        len_ = curr_offset;
    }

    void beginTuple() override {
        // Projection 本身不负责扫描数据，只要求子执行器定位到第一条结果。
        // 子执行器定位好后，Projection 的 Next() 才能从那条完整记录中裁剪列。
        prev_->beginTuple();
    }

    void nextTuple() override {
        // 投影结果与子节点结果一一对应，下一条投影结果就是子节点的下一条结果。
        prev_->nextTuple();
    }

    std::unique_ptr<RmRecord> Next() override {
        if (is_end()) {
            return nullptr;
        }

        // 先取子节点当前完整记录，再把被 select 的列拷贝到新的结果记录中。
        auto prev_rec = prev_->Next();
        auto projected = std::make_unique<RmRecord>(len_);
        auto &prev_cols = prev_->cols();

        for (size_t i = 0; i < sel_idxs_.size(); ++i) {
            const ColMeta &src_col = prev_cols[sel_idxs_[i]];
            const ColMeta &dst_col = cols_[i];
            // src_col.offset 是字段在子记录中的位置，dst_col.offset 是字段在投影结果中的新位置。
            // memcpy 只复制该字段的字节数 src_col.len，因此不会把未选择的列带进结果。
            memcpy(projected->data + dst_col.offset, prev_rec->data + src_col.offset, src_col.len);
        }
        return projected;
    }

    // 投影算子本身不直接对应表中的某个 Rid，因此返回抽象 rid 占位。
    Rid &rid() override { return _abstract_rid; }

    // Projection 是否结束完全取决于它的子执行器是否结束。
    bool is_end() const override { return prev_->is_end(); }
    
    std::string getType() override { return "ProjectionExecutor"; }

    size_t tupleLen() const override { return len_; }

    const std::vector<ColMeta> &cols() const override { return cols_; }
};



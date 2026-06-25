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
 * 验收讲解：UpdateExecutor 执行 update 语句。
 * Portal 已经先扫描出满足 where 条件的 Rid 列表 rids_，所以这里不再判断 where，只按 Rid 取旧记录、改 set 指定字段、再写回原位置。
 * 先复制整条旧记录再覆盖字段，是为了保留没有被 set 修改的其他列。
 * update 属于 DML 修改语句，不产生结果表，所以 Next() 执行完修改后返回 nullptr。
 */

class UpdateExecutor : public AbstractExecutor {
   private:
    TabMeta tab_;                         // 被更新表的元数据，用来查每个 set 字段的类型、长度、offset。
    std::vector<Condition> conds_;        // update 的 where 条件；传入这里主要保留语义信息，真正筛选已经由 Portal 完成。
    RmFileHandle *fh_;                    // 被更新表的数据文件句柄，负责 get_record 和 update_record。
    std::vector<Rid> rids_;               // 所有需要更新的记录位置；每个 Rid 精确定位到 page_no + slot_no。
    std::string tab_name_;                // 表名，便于调试和理解当前执行器操作的是哪张表。
    std::vector<SetClause> set_clauses_;  // set 子句列表，例如 set age = 20, name = 'Tom'。
    SmManager *sm_manager_;               // 系统管理器，用来取得表元数据和表文件句柄。

   public:
    UpdateExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<SetClause> set_clauses,
                   std::vector<Condition> conds, std::vector<Rid> rids, Context *context) {
        // 构造阶段只保存执行 update 所需的信息；真正修改发生在 Next() 中。
        sm_manager_ = sm_manager;
        tab_name_ = tab_name;
        set_clauses_ = set_clauses;
        // tab_ 中的 ColMeta 告诉我们每个字段在记录 data 中从哪里开始、占多少字节。
        tab_ = sm_manager_->db_.get_table(tab_name);
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        conds_ = conds;
        rids_ = rids;
        context_ = context;
    }
    std::unique_ptr<RmRecord> Next() override {
        // Portal 已经通过扫描算子把满足 where 条件的记录位置收集到 rids_ 中。
        // UpdateExecutor 只需要逐个取出旧记录，修改 set 子句指定的列，再写回原 rid。
        for (const Rid &rid : rids_) {
            auto record = fh_->get_record(rid, context_);
            // new_record 是旧记录的完整副本。后面只覆盖 set 指定的列，未被修改的列仍保持旧值。
            // 不能只创建空记录再写 set 字段，否则未被 set 的列会丢失原值。
            RmRecord new_record(record->size, record->data);

            for (auto &set_clause : set_clauses_) {
                // 根据 set 左侧列名找到列元数据，例如 set age = 20 就找到 age 这一列。
                auto col = tab_.get_col(set_clause.lhs.col_name);
                // 类型必须一致，例如 int 列不能直接写入 string 值；不一致就抛 IncompatibleTypeError。
                if (col->type != set_clause.rhs.type) {
                    throw IncompatibleTypeError(coltype2str(col->type), coltype2str(set_clause.rhs.type));
                }
                // rhs.raw 是右值的二进制形式。如果还没初始化，就按目标列长度生成 raw。
                if (set_clause.rhs.raw == nullptr) {
                    set_clause.rhs.init_raw(col->len);
                }
                // col->offset 指向该列在整条记录中的起始位置，col->len 是该字段占用字节数。
                // 因此这里覆盖的是单个字段的字节区域，不会影响记录中的其他字段。
                memcpy(new_record.data + col->offset, set_clause.rhs.raw->data, col->len);
            }

            // update_record 会把新字节写回原来的 rid 位置，并把对应页标记为脏页，之后由缓冲池刷盘。
            fh_->update_record(rid, new_record.data, context_);
        }
        return nullptr;
    }

    // update 不产生可供上层读取的表记录，因此返回抽象 rid 占位。
    Rid &rid() override { return _abstract_rid; }
};



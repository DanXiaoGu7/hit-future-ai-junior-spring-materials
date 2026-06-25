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
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

/*
 * 验收讲解：DeleteExecutor 执行 delete 语句。
 * rids_ 中保存所有要删除的记录位置；这些 Rid 已经由 Portal 根据 where 条件提前筛选好。
 * 真正删除时调用 Lab2 的 RmFileHandle::delete_record，由记录管理器负责清 bitmap、维护页内记录数和空闲页链表。
 * delete 不产生查询结果，所以 Next() 执行完删除动作后返回 nullptr。
 */

class DeleteExecutor : public AbstractExecutor {
   private:
    TabMeta tab_;                   // 表的元数据；当前删除逻辑不直接使用它，但保留下来便于后续维护索引删除。
    std::vector<Condition> conds_;  // delete 的 where 条件；真正过滤已经在构造 rids_ 之前完成。
    RmFileHandle *fh_;              // 表的数据文件句柄，delete_record 通过它修改记录文件。
    std::vector<Rid> rids_;         // 需要删除的记录位置；Rid = page_no + slot_no，可以精确定位一条记录。
    std::string tab_name_;          // 表名称，说明当前执行器删除的是哪张表的数据。
    SmManager *sm_manager_;         // 系统管理器，用来获取表元数据和表文件句柄。

   public:
    DeleteExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<Condition> conds,
                   std::vector<Rid> rids, Context *context) {
        // 构造阶段只保存删除所需的信息；真正删除发生在 Next() 中。
        sm_manager_ = sm_manager;
        tab_name_ = tab_name;
        tab_ = sm_manager_->db_.get_table(tab_name);
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        conds_ = conds;
        rids_ = rids;
        context_ = context;
    }

    std::unique_ptr<RmRecord> Next() override {
        // Portal 已经通过扫描算子把满足 where 条件的记录位置收集到 rids_ 中。
        // 删除时只需按 rid 调用记录管理器接口即可，不需要重新判断 where。
        for (const Rid &rid : rids_) {
            // 这里不直接清理 slot 字节，而是交给记录管理器 delete_record。
            // delete_record 会把 bitmap 对应位清 0，并在必要时把页面挂回空闲页链表。
            // 这样之后插入新记录时，可以复用刚刚释放出来的 slot。
            fh_->delete_record(rid, context_);
        }
        return nullptr;
    }

    // delete 不产生可供上层读取的表记录，因此返回抽象 rid 占位。
    Rid &rid() override { return _abstract_rid; }
};


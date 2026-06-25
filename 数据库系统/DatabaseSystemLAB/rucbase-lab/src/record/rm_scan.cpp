/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_scan.h"
#include "rm_file_handle.h"

/*
 * 验收讲解：RmScan 是记录文件的顺序扫描器。
 *
 * 它要解决的问题是：一张表中哪些 slot 当前有有效记录？
 * 答案不靠直接看 slot 字节，而是靠每个记录页中的 bitmap。
 *
 * 扫描器内部只保存一个当前 Rid：
 * - rid_.page_no 表示当前扫描到哪一页。
 * - rid_.slot_no 表示当前扫描到该页的哪个槽位。
 *
 * 每次 next() 都从当前 Rid 的下一个 slot 开始找 bitmap 为 1 的位置。
 * 如果当前页找不到，就移动到下一页；如果所有页都找完，就把 rid_ 设为 RM_NO_PAGE 表示结束。
 */

/**
 * @brief 初始化 file_handle 和 rid
 * @param file_handle 被扫描的记录文件句柄
 */
RmScan::RmScan(const RmFileHandle *file_handle) : file_handle_(file_handle) {
    // 扫描从第一张记录页开始。第 0 页是文件头，不存真实记录。
    // slot_no 初始化为 -1，是为了让第一次 next() 从 slot 0 开始找。
    rid_ = {RM_FIRST_RECORD_PAGE, -1};

    // 构造完成后，扫描器直接定位到第一条有效记录。
    // 如果文件中没有任何有效记录，next() 会把 rid_ 设置为 {RM_NO_PAGE, -1}。
    next();
}

/**
 * @brief 找到文件中下一个存放了记录的位置
 */
void RmScan::next() {
    // 外层 while 按页向后扫描。
    // file_hdr_.num_pages 是文件当前总页数，page_no 达到它说明已经越过最后一页。
    while (rid_.page_no < file_handle_->file_hdr_.num_pages) {
        // 取出当前页。fetch_page_handle 会 pin 页面，所以本轮检查完必须 unpin。
        RmPageHandle page_handle = file_handle_->fetch_page_handle(rid_.page_no);

        // 在当前页 bitmap 中寻找下一个为 1 的 bit。
        // 参数含义：
        // - true：要找的目标 bit 值是 1，也就是有效记录。
        // - page_handle.bitmap：当前页的 bitmap 起始地址。
        // - num_records_per_page：本页最多有多少个 slot，也是 bitmap 需要检查的 bit 数。
        // - rid_.slot_no：当前 slot，next_bit 会从它后面继续找，避免重复返回同一条记录。
        int next_slot = Bitmap::next_bit(
            true,
            page_handle.bitmap,
            file_handle_->file_hdr_.num_records_per_page,
            rid_.slot_no
        );

        // 本函数只读 bitmap，不修改页面，所以 dirty=false。
        file_handle_->buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);

        if (next_slot < file_handle_->file_hdr_.num_records_per_page) {
            // 找到了有效 slot：更新当前 rid_，让扫描器停在这条记录上。
            // 上层可以通过 scan.rid() 得到这个位置，再调用 get_record 读取记录内容。
            rid_.slot_no = next_slot;
            return;
        }

        // 当前页从 rid_.slot_no 后面已经没有有效记录了。
        // 移动到下一页，并把 slot_no 置为 -1，表示下一页要从 slot 0 前面开始找。
        rid_.page_no++;
        rid_.slot_no = -1;
    }

    // 所有记录页都扫描完了。
    // 用 RM_NO_PAGE 作为结束哨兵，is_end() 只需要检查 page_no 即可。
    rid_ = {RM_NO_PAGE, -1};
}

/**
 * @brief 判断是否到达文件末尾
 */
bool RmScan::is_end() const {
    return rid_.page_no == RM_NO_PAGE;
}

/**
 * @brief RmScan 内部存放的 rid
 */
Rid RmScan::rid() const {
    return rid_;
}

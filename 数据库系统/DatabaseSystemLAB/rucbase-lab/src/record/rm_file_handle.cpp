/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_file_handle.h"

/*
 * 验收讲解：本文件实现“表文件中一条条记录怎么放、怎么找、怎么改”。
 *
 * 一张表在底层对应一个记录文件。这个记录文件不是简单地把记录连续写在一起，而是按“页”组织：
 * 1. 第 0 页：保存 RmFileHdr，也就是整个记录文件的文件头。
 * 2. 第 1 页及以后：保存真实记录，每一页内部又分成 page header、bitmap、slots 三部分。
 *
 * 每个数据页内部结构可以理解成：
 *
 *     [RmPageHdr][bitmap][slot0][slot1][slot2] ...
 *
 * - RmPageHdr 记录这一页当前有多少条记录，以及它在空闲页链表中的下一个页号。
 * - bitmap 的每一位对应一个 slot，1 表示该 slot 有有效记录，0 表示该 slot 空闲。
 * - slots 区域保存记录的原始字节。本实验中记录是定长的，所以 slot 地址可以直接通过 slot_no 计算。
 *
 * 本文件最重要的三件事：
 * 1. 用 Rid(page_no, slot_no) 定位记录。
 * 2. 用 bitmap 判断 slot 是否真的有效。
 * 3. 用 first_free_page_no 和 next_free_page_no 维护“还有空槽的页”的链表。
 *
 * 与 BufferPoolManager 的关系：
 * - fetch_page_handle/new_page 会让页面进入 pin 状态，表示正在使用，不能被替换。
 * - 用完页面后必须 unpin，否则 pin_count 不能下降，缓冲池可能无法淘汰页面。
 * - 只读页面时 unpin 的 dirty 参数传 false；修改了页头、bitmap 或 slot 数据时传 true。
 */

/**
 * @description: 获取当前表中记录号为 rid 的记录
 * @param {Rid&} rid 记录号，指定记录的位置
 * @param {Context*} context 当前上下文，本实验这个函数里暂时没有用到
 * @return {unique_ptr<RmRecord>} rid 对应的记录对象指针
 */
std::unique_ptr<RmRecord> RmFileHandle::get_record(const Rid& rid, Context* context) const {
    // Rid 由 page_no 和 slot_no 组成。
    // page_no 定位记录所在的数据页；slot_no 定位该页内的第几个记录槽。
    // fetch_page_handle 会通过 BufferPoolManager 把目标页取到缓冲池，并 pin 住该页。
    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    // 只要传入的 Rid 不可信，就必须做两类检查：
    // 1. slot_no 是否越界。slot_no 小于 0 或超过每页最大记录数，都不可能是合法记录。
    // 2. bitmap 对应位是否为 1。bitmap 为 0 表示这个 slot 当前没有有效记录，即使 slot 里还残留旧字节也不能读。
    if (rid.slot_no < 0 || rid.slot_no >= file_hdr_.num_records_per_page ||
        !Bitmap::is_set(page_handle.bitmap, rid.slot_no)) {
        // 这里没有修改页面，只是检查失败，所以 dirty=false。
        // 抛异常前也必须 unpin，否则异常路径会泄漏 pin_count。
        buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // page_handle.get_slot(rid.slot_no) 得到的是缓冲池页面内部的 slot 地址。
    // RmRecord(size, data) 会重新分配一块内存，并把 slot 中的记录字节拷贝出来。
    // 这样返回给上层的是独立副本，不依赖缓冲池页面继续留在内存中。
    auto record = std::make_unique<RmRecord>(file_hdr_.record_size, page_handle.get_slot(rid.slot_no));

    // get_record 只是读，不改页头、不改 bitmap、不改 slot，所以 dirty=false。
    buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
    return record;
}

/**
 * @description: 在当前表中插入一条记录，不指定插入位置
 * @param {char*} buf 要插入的记录数据，长度应该等于 file_hdr_.record_size
 * @param {Context*} context 当前上下文，本实验这个函数里暂时没有用到
 * @return {Rid} 插入记录的位置，之后可以通过这个 Rid 找回该记录
 */
Rid RmFileHandle::insert_record(char* buf, Context* context) {
    // create_page_handle 的语义是：找一张“还有空槽”的页。
    // 如果 file_hdr_.first_free_page_no == RM_NO_PAGE，说明当前没有空闲页，它会新建一页。
    // 返回的 page_handle 对应页面已经被 pin，函数结束前必须 unpin。
    RmPageHandle page_handle = create_page_handle();

    // bitmap 中 false/0 表示空槽。
    // first_bit(false, ...) 会返回第一个空 slot 的编号。
    int slot_no = Bitmap::first_bit(false, page_handle.bitmap, file_hdr_.num_records_per_page);
    if (slot_no == file_hdr_.num_records_per_page) {
        // 理论上 create_page_handle 返回的页应该一定有空槽。
        // 如果没找到空槽，说明空闲页链表元数据被破坏，抛 InternalError 更合适。
        buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
        throw InternalError("RmFileHandle::insert_record: no free slot in free page");
    }

    // 真正写入记录分三步：
    // 1. 把 buf 中的定长记录字节复制到 slot 中。
    // 2. 把 bitmap 对应位置 1，表示该 slot 已经有效。
    // 3. 页头 num_records 加一，记录当前页有效记录数增加。
    memcpy(page_handle.get_slot(slot_no), buf, file_hdr_.record_size);
    Bitmap::set(page_handle.bitmap, slot_no);
    page_handle.page_hdr->num_records++;

    // Rid 只记录“位置”，不保存数据内容。
    // page_no 来自当前 page 的 PageId，slot_no 是刚才在 bitmap 中找到的空槽编号。
    Rid rid{page_handle.page->get_page_id().page_no, slot_no};

    // 如果插入后页面变满，它就不应该继续留在空闲页链表里。
    // 由于 create_page_handle 普通插入总是取 first_free_page_no 指向的链表头页，
    // 所以移除时只需要把文件头 first_free_page_no 改成当前页的 next_free_page_no。
    if (page_handle.page_hdr->num_records == file_hdr_.num_records_per_page) {
        file_hdr_.first_free_page_no = page_handle.page_hdr->next_free_page_no;
        page_handle.page_hdr->next_free_page_no = RM_NO_PAGE;
    }

    // 插入同时修改了 slot 数据、bitmap、页头，可能还修改了文件头 first_free_page_no。
    // 因此页面必须标记 dirty。文件头 file_hdr_ 会在 RmManager::close_file 时写回文件头页。
    buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), true);
    return rid;
}

/**
 * @description: 在当前表中的指定位置插入一条记录
 * @param {Rid&} rid 要插入记录的位置
 * @param {char*} buf 要插入记录的数据
 */
void RmFileHandle::insert_record(const Rid& rid, char* buf) {
    // 指定位置插入主要供恢复或后续模块使用。
    // 与普通插入不同，它不是从 first_free_page_no 取链表头，而是直接使用调用者给的 Rid。
    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    // 指定插入可以覆盖已有 slot 的内容，也可以填入空 slot。
    // 但 slot_no 本身必须在合法范围内。
    if (rid.slot_no < 0 || rid.slot_no >= file_hdr_.num_records_per_page) {
        buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // was_empty 用来判断这个位置原来是不是空槽。
    // 如果原来为空，插入后有效记录数要加一；如果原来已有记录，则只是覆盖内容，不改变 num_records。
    bool was_empty = !Bitmap::is_set(page_handle.bitmap, rid.slot_no);
    if (was_empty) {
        Bitmap::set(page_handle.bitmap, rid.slot_no);
        page_handle.page_hdr->num_records++;
    }
    memcpy(page_handle.get_slot(rid.slot_no), buf, file_hdr_.record_size);

    // 如果这次指定插入让页面从“有空槽”变成“满页”，就必须从空闲页链表中摘除。
    // 普通插入只会操作链表头；指定 Rid 插入可能命中链表中间的某一页，所以要查找它的前驱页。
    if (was_empty && page_handle.page_hdr->num_records == file_hdr_.num_records_per_page) {
        if (file_hdr_.first_free_page_no == rid.page_no) {
            // 当前页正好是空闲页链表头，直接移动链表头。
            file_hdr_.first_free_page_no = page_handle.page_hdr->next_free_page_no;
        } else {
            // 当前页不在链表头，需要从 first_free_page_no 开始沿 next_free_page_no 找前驱页。
            int prev_page_no = file_hdr_.first_free_page_no;
            while (prev_page_no != RM_NO_PAGE) {
                RmPageHandle prev_handle = fetch_page_handle(prev_page_no);
                if (prev_handle.page_hdr->next_free_page_no == rid.page_no) {
                    // 找到前驱后，让前驱跳过当前满页，指向当前页的下一个空闲页。
                    prev_handle.page_hdr->next_free_page_no = page_handle.page_hdr->next_free_page_no;
                    buffer_pool_manager_->unpin_page(prev_handle.page->get_page_id(), true);
                    break;
                }
                int next_page_no = prev_handle.page_hdr->next_free_page_no;
                buffer_pool_manager_->unpin_page(prev_handle.page->get_page_id(), false);
                prev_page_no = next_page_no;
            }
        }
        // 当前页已满，不再属于空闲页链表，next_free_page_no 置为 RM_NO_PAGE 更清晰。
        page_handle.page_hdr->next_free_page_no = RM_NO_PAGE;
    }

    buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), true);
}

/**
 * @description: 删除记录文件中记录号为 rid 的记录
 * @param {Rid&} rid 要删除的记录的位置
 * @param {Context*} context 当前上下文，本实验这个函数里暂时没有用到
 */
void RmFileHandle::delete_record(const Rid& rid, Context* context) {
    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    // 删除前也必须验证 Rid 是否真的指向有效记录。
    // 如果 bitmap 为 0，说明这条记录已经不存在，重复删除应该报 RecordNotFoundError。
    if (rid.slot_no < 0 || rid.slot_no >= file_hdr_.num_records_per_page ||
        !Bitmap::is_set(page_handle.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // was_full 记录删除前页面是否满。
    // 只有满页原来不在空闲页链表中；删除后它才需要重新加入链表。
    bool was_full = page_handle.page_hdr->num_records == file_hdr_.num_records_per_page;

    // 删除记录不必清空 slot 字节，只要 bitmap 清 0 即可。
    // 后续扫描、读取都会先看 bitmap；bitmap 为 0 的 slot 会被当作空槽。
    Bitmap::reset(page_handle.bitmap, rid.slot_no);
    page_handle.page_hdr->num_records--;

    if (was_full) {
        // 删除前是满页，删除后出现空槽，所以需要放回空闲页链表，供以后插入复用。
        release_page_handle(page_handle);
    }

    buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), true);
}


/**
 * @description: 更新记录文件中记录号为 rid 的记录
 * @param {Rid&} rid 要更新的记录的位置
 * @param {char*} buf 新记录的数据，长度应该等于 file_hdr_.record_size
 * @param {Context*} context 当前上下文，本实验这个函数里暂时没有用到
 */
void RmFileHandle::update_record(const Rid& rid, char* buf, Context* context) {
    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    // 更新只能发生在已有记录上。
    // 如果 slot 不存在或 bitmap 为 0，就不能把它当作有效记录更新。
    if (rid.slot_no < 0 || rid.slot_no >= file_hdr_.num_records_per_page ||
        !Bitmap::is_set(page_handle.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), false);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // 本实验采用定长记录，所以更新时直接覆盖原 slot 即可。
    // 不需要像变长记录那样考虑空间是否够、是否移动记录、是否更新偏移目录。
    memcpy(page_handle.get_slot(rid.slot_no), buf, file_hdr_.record_size);
    buffer_pool_manager_->unpin_page(page_handle.page->get_page_id(), true);
}

/**
 * 以下函数为辅助函数，仅提供参考，可以选择完成如下函数，也可以删除如下函数，在单元测试中不涉及如下函数接口的直接调用
*/
/**
 * @description: 获取指定页面的页面句柄
 * @param {int} page_no 页面号
 * @return {RmPageHandle} 指定页面的句柄
 */
RmPageHandle RmFileHandle::fetch_page_handle(int page_no) const {
    // 记录文件的第 0 页保存 RmFileHdr，不存真实记录。
    // RM_FIRST_RECORD_PAGE 通常为 1，所以真实记录页必须满足 page_no >= 1。
    // page_no >= file_hdr_.num_pages 表示这个页号还没有被分配出来。
    if (page_no < RM_FIRST_RECORD_PAGE || page_no >= file_hdr_.num_pages) {
        throw PageNotExistError(disk_manager_->get_file_name(fd_), page_no);
    }

    // PageId 用 fd + page_no 唯一标识一个页面。
    // 只用 page_no 不够，因为不同表文件都可能有第 1 页、第 2 页。
    PageId page_id{fd_, page_no};
    Page *page = buffer_pool_manager_->fetch_page(page_id);
    if (page == nullptr) {
        throw InternalError("RmFileHandle::fetch_page_handle: failed to fetch page");
    }

    // RmPageHandle 不拥有页面，只是把 Page* 按记录页格式解释，方便访问 page_hdr、bitmap、slots。
    return RmPageHandle(&file_hdr_, page);
}

/**
 * @description: 创建一个新的 page handle
 * @return {RmPageHandle} 新的 PageHandle
 */
RmPageHandle RmFileHandle::create_new_page_handle() {
    // INVALID_PAGE_ID 表示让 BufferPoolManager/DiskManager 自动分配一个新的 page_no。
    PageId page_id{fd_, INVALID_PAGE_ID};
    Page *page = buffer_pool_manager_->new_page(&page_id);
    if (page == nullptr) {
        throw InternalError("RmFileHandle::create_new_page_handle: failed to create page");
    }

    RmPageHandle page_handle(&file_hdr_, page);

    // 新页刚创建出来时没有任何记录：
    // - num_records = 0。
    // - bitmap 全部清 0，表示所有 slot 都空闲。
    // - next_free_page_no 指向旧的空闲页链表头，准备把新页插到链表头。
    page_handle.page_hdr->num_records = 0;
    page_handle.page_hdr->next_free_page_no = file_hdr_.first_free_page_no;
    Bitmap::init(page_handle.bitmap, file_hdr_.bitmap_size);

    // new_page 已经分配出新的 page_no，所以文件总页数加一。
    // 新页一定有空槽，因此把它设为新的空闲页链表头。
    file_hdr_.num_pages++;
    file_hdr_.first_free_page_no = page_id.page_no;

    // 注意：这里不 unpin。调用者拿到新页后通常马上要写入记录，最后由调用者统一 unpin。
    return page_handle;
}

/**
 * @brief 创建或获取一个空闲的 page handle
 *
 * @return RmPageHandle 返回生成的空闲 page handle
 * @note pin the page, remember to unpin it outside!
 */
RmPageHandle RmFileHandle::create_page_handle() {
    // first_free_page_no 维护的是“还有空槽的数据页”链表头。
    // 如果它等于 RM_NO_PAGE，说明所有已有页都满了，或者当前还没有记录页，需要创建新页。
    if (file_hdr_.first_free_page_no == RM_NO_PAGE) {
        return create_new_page_handle();
    }

    // 否则直接取空闲页链表头对应的页。
    // 这个页至少应该有一个空 slot，insert_record 会在 bitmap 中继续找具体 slot。
    return fetch_page_handle(file_hdr_.first_free_page_no);
}

/**
 * @description: 当一个页面从没有空闲空间的状态变为有空闲空间状态时，更新文件头和页头中空闲页面相关的元数据
 */
void RmFileHandle::release_page_handle(RmPageHandle&page_handle) {
    // 这个函数通常在“满页删除了一条记录”后调用。
    // 满页原来不在空闲页链表中；删除后它有了空 slot，就应该重新加入空闲页链表。
    // 采用头插法：当前页的 next 指向旧链表头，文件头 first_free_page_no 改成当前页号。
    page_handle.page_hdr->next_free_page_no = file_hdr_.first_free_page_no;
    file_hdr_.first_free_page_no = page_handle.page->get_page_id().page_no;
}

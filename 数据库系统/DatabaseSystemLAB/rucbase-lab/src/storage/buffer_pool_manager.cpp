/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "buffer_pool_manager.h"

/*
 * 本文件实现 BufferPoolManager，即缓冲池管理器。
 *
 * 可以把缓冲池理解成“磁盘页的内存缓存”：
 * - 磁盘上的数据按 page 存储，读写磁盘很慢。
 * - 内存中的缓冲池由很多 frame 组成，一个 frame 可以缓存一个 page。
 * - 如果要访问的 page 已经在缓冲池里，就直接返回内存中的 Page。
 * - 如果不在缓冲池里，就从磁盘读入一个 frame。
 * - 如果缓冲池满了，就必须找一个没人使用的 frame 淘汰掉。
 *
 * 本类主要维护四类状态：
 * 1. page_table_：PageId -> frame_id。用 PageId 快速找到页面在缓冲池哪个 frame。
 * 2. free_list_：当前空闲的 frame 列表。空闲 frame 没装有效页面，可以直接用。
 * 3. Page::pin_count_：页面正在被多少调用者使用。大于 0 表示不能淘汰。
 * 4. Page::is_dirty_：页面是否被修改过。脏页淘汰前必须写回磁盘。
 *
 * 验收时可以抓住一句话：
 * BufferPoolManager 负责“找 frame、维护页表、维护 pin_count、淘汰前写回脏页”。
 */

bool BufferPoolManager::find_victim_page(frame_id_t* frame_id) {
    // 这个辅助函数只回答一个问题：
    // “现在有没有一个 frame 可以拿来放页面？”
    //
    // 它不负责读磁盘，也不负责写磁盘，只返回可用 frame 的编号。

    // 缓冲池找可用 frame 有两个来源：
    // 1. free_list_：从未使用或已经 delete_page 释放的空闲 frame。
    // 2. replacer_：缓冲池满时，由 LRU 选择 pin_count 为 0 的可淘汰 frame。
    //
    // 必须优先使用 free_list_，因为空闲 frame 不需要淘汰旧页，也不需要处理写回。
    if (!free_list_.empty()) {
        *frame_id = free_list_.front();
        free_list_.pop_front();
        return true;
    }

    // free_list_ 为空说明所有 frame 都装过页面。
    // 此时只能从 replacer_ 中找可淘汰页；如果所有页都被 pin，victim 会返回 false。
    return replacer_->victim(frame_id);
}

Page* BufferPoolManager::fetch_page(PageId page_id) {
    // fetch_page 的作用：获取一个已经存在的磁盘页。
    //
    // 它有两种可能：
    // 1. 页面已经在缓冲池：直接返回内存页。
    // 2. 页面不在缓冲池：找 frame，从磁盘 read_page 读进来。

    // BufferPoolManager 的页表、空闲链表、页面元数据都是共享状态，
    // 所以对外接口统一加锁，保证并发测试下不会出现状态竞争。
    std::scoped_lock lock{latch_};

    // INVALID_PAGE_ID 不是一个真实磁盘页，不能 fetch。
    if (page_id.page_no == INVALID_PAGE_ID) {
        return nullptr;
    }

    // 情况 1：目标页已经在缓冲池中。
    // page_table_ 保存 PageId -> frame_id 的映射，可以 O(1) 定位页面所在 frame。
    //
    // 这也是 page_table_ 的核心作用：避免每次都遍历整个 pages_ 数组查找。
    auto table_it = page_table_.find(page_id);
    if (table_it != page_table_.end()) {
        Page* page = &pages_[table_it->second];

        // fetch 表示上层开始使用该页面，所以 pin_count_ 加 1。
        // pin_count_ > 0 的页面不能被淘汰。
        //
        // 例如同一个页面被两个地方同时 fetch，pin_count_ 就可能是 2。
        // 只有两个地方都 unpin 后，它才能重新进入 LRU。
        page->pin_count_++;

        // 如果这个页面之前 pin_count 为 0，它可能在 replacer_ 里。
        // 现在重新被使用，需要从可淘汰集合中移除。
        replacer_->pin(table_it->second);
        return page;
    }

    // 情况 2：目标页不在缓冲池中，需要找一个 frame 装入磁盘页。
    // 如果没有空闲 frame，且 LRU 也找不到可淘汰 frame，说明所有页面都被 pin，fetch 失败。
    frame_id_t frame_id = INVALID_FRAME_ID;
    if (!find_victim_page(&frame_id)) {
        return nullptr;
    }

    Page* page = &pages_[frame_id];

    // 如果选中的 frame 原来装着一个有效页面，需要先处理旧页。
    // 注意：从 free_list_ 取出的全新 frame 的 page_no 通常是 INVALID_PAGE_ID。
    if (page->id_.page_no != INVALID_PAGE_ID) {
        // 旧页是脏页时，说明内存内容比磁盘新。
        // 淘汰前必须写回磁盘，否则之前的修改会丢失。
        if (page->is_dirty_) {
            disk_manager_->write_page(page->id_.fd, page->id_.page_no, page->data_, PAGE_SIZE);
        }

        // 旧页即将离开缓冲池，页表映射必须删除。
        // 否则后续 fetch 旧 PageId 会错误地定位到这个 frame。
        //
        // 举例：frame 5 原来存 page A，现在要改存 page B。
        // 如果不删除 page A 的映射，page_table_ 会误以为 page A 还在 frame 5。
        page_table_.erase(page->id_);
    }

    // 清空 frame 中的旧数据，然后从磁盘读入目标页。
    page->reset_memory();
    disk_manager_->read_page(page_id.fd, page_id.page_no, page->data_, PAGE_SIZE);

    // 更新页面元数据和页表。
    // 新读入的页面正在被 fetch 调用者使用，所以 pin_count_ 初始化为 1。
    // 刚从磁盘读入，与磁盘一致，所以 is_dirty_ 为 false。
    page->id_ = page_id;
    page->pin_count_ = 1;
    page->is_dirty_ = false;
    page_table_[page_id] = frame_id;

    // 防御性调用 pin，确保该 frame 不在 replacer_ 中。
    replacer_->pin(frame_id);
    return page;
}

bool BufferPoolManager::unpin_page(PageId page_id, bool is_dirty) {
    // unpin_page 的作用：告诉缓冲池“我暂时用完这个页面了”。
    //
    // 注意：unpin 不是删除页面，也不是写回页面。
    // 它只是把 pin_count_ 减 1；只有减到 0 时，页面才可以被 LRU 淘汰。
    std::scoped_lock lock{latch_};

    // 只有在缓冲池中的页面才能 unpin。
    auto table_it = page_table_.find(page_id);
    if (table_it == page_table_.end()) {
        return false;
    }

    Page* page = &pages_[table_it->second];

    // pin_count_ 已经为 0 表示没有使用者，继续 unpin 是非法操作。
    if (page->pin_count_ <= 0) {
        return false;
    }

    // is_dirty 是本次调用者告诉缓冲池“我是否修改了页面”。
    // 只能在 true 时置脏，不能在 false 时清脏：
    // 因为页面可能之前已经被其他调用修改过，false 只表示本次没有新修改。
    //
    // is_dirty=false 不能写成 page->is_dirty_=false，否则会把之前的修改标记清掉。
    if (is_dirty) {
        page->is_dirty_ = true;
    }

    // 每次 unpin 只释放一次使用引用。
    page->pin_count_--;

    // 只有 pin_count_ 变成 0，页面才真正没人使用，才能交给 LRU 管理。
    if (page->pin_count_ == 0) {
        replacer_->unpin(table_it->second);
    }
    return true;
}

bool BufferPoolManager::flush_page(PageId page_id) {
    // flush_page 的作用：把缓冲池中某一页的内存内容写回磁盘。
    // 它不改变 pin_count_，也不把页面从缓冲池中删除。
    std::scoped_lock lock{latch_};

    // flush_page 只能刷新当前在缓冲池中的有效页面。
    auto table_it = page_table_.find(page_id);
    if (table_it == page_table_.end() || page_id.page_no == INVALID_PAGE_ID) {
        return false;
    }

    Page* page = &pages_[table_it->second];

    // 实验要求 flush 是强制刷盘：
    // 不管页面是否为脏页，不管 pin_count_ 是否大于 0，都写回磁盘。
    disk_manager_->write_page(page->id_.fd, page->id_.page_no, page->data_, PAGE_SIZE);

    // 写回后内存和磁盘一致，可以清除脏页标记。
    page->is_dirty_ = false;
    return true;
}

Page* BufferPoolManager::new_page(PageId* page_id) {
    // new_page 的作用：创建一个新的磁盘页，并把这个新页放进缓冲池。
    //
    // 它和 fetch_page 的区别：
    // - fetch_page 是读取“已经存在的 page_no”。
    // - new_page 是先 allocate_page 分配一个新的 page_no。
    std::scoped_lock lock{latch_};

    // 新建页面同样需要一个 frame。
    // 如果所有 frame 都被 pin，无法淘汰，new_page 失败。
    frame_id_t frame_id = INVALID_FRAME_ID;
    if (!find_victim_page(&frame_id)) {
        return nullptr;
    }

    Page* page = &pages_[frame_id];

    // 如果该 frame 原来保存了有效旧页，需要先处理旧页再复用 frame。
    if (page->id_.page_no != INVALID_PAGE_ID) {
        // 脏页淘汰前必须写回，保证数据持久化。
        if (page->is_dirty_) {
            disk_manager_->write_page(page->id_.fd, page->id_.page_no, page->data_, PAGE_SIZE);
        }

        // 删除旧页的页表映射，避免旧 PageId 继续指向这个已复用的 frame。
        page_table_.erase(page->id_);
    }

    // DiskManager 按文件 fd 自增分配 page_no。
    // 传入的 page_id 已经带有 fd，这里补全新页面编号。
    //
    // PageId 由 fd 和 page_no 组成：
    // - fd 表示这个页属于哪个打开的文件。
    // - page_no 表示它是该文件里的第几页。
    page_id->page_no = disk_manager_->allocate_page(page_id->fd);

    // 新页开始时是空白页，先重置内存，再设置元数据。
    page->reset_memory();
    page->id_ = *page_id;

    // new_page 返回的页面马上交给调用者写入，所以 pin_count_ 为 1。
    // 此时页面内容刚初始化为空白，和下面写入磁盘后的内容一致，所以不是脏页。
    page->pin_count_ = 1;
    page->is_dirty_ = false;

    // 建立新 PageId 到 frame 的映射。
    page_table_[*page_id] = frame_id;

    // 新页正在被调用者使用，不能被 LRU 淘汰。
    replacer_->pin(frame_id);

    // 立即把空白页写入磁盘。
    // 这样即使后续该页未被修改就被 fetch，也能从磁盘读到完整 PAGE_SIZE 数据。
    disk_manager_->write_page(page_id->fd, page_id->page_no, page->data_, PAGE_SIZE);
    return page;
}

bool BufferPoolManager::delete_page(PageId page_id) {
    // delete_page 的作用：释放缓冲池中某个页面占用的 frame。
    //
    // 这里的“删除”主要是清理缓冲池状态，并把 frame 放回 free_list_。
    // DiskManager::deallocate_page 在当前实验中是空实现，所以磁盘页回收不是重点。
    std::scoped_lock lock{latch_};

    // 如果页面不在缓冲池中，缓冲池内没有状态需要清理。
    // DiskManager::deallocate_page 当前是空实现，但保留调用符合接口语义。
    auto table_it = page_table_.find(page_id);
    if (table_it == page_table_.end()) {
        disk_manager_->deallocate_page(page_id.page_no);
        return true;
    }

    frame_id_t frame_id = table_it->second;
    Page* page = &pages_[frame_id];

    // 仍被使用的页面不能删除，否则调用者手中的 Page* 会失效。
    if (page->pin_count_ != 0) {
        return false;
    }

    // 删除前写回页面，保证已修改的数据不会因为释放 frame 而丢失。
    disk_manager_->write_page(page->id_.fd, page->id_.page_no, page->data_, PAGE_SIZE);
    disk_manager_->deallocate_page(page_id.page_no);

    // 页面从缓冲池移除，页表和 replacer 都要同步清理。
    page_table_.erase(table_it);
    replacer_->pin(frame_id);

    // 重置 frame，使它重新变成空闲 frame。
    page->reset_memory();
    page->id_.fd = INVALID_PAGE_ID;
    page->id_.page_no = INVALID_PAGE_ID;
    page->pin_count_ = 0;
    page->is_dirty_ = false;

    // 释放后的 frame 放回 free_list_，后续优先复用。
    free_list_.push_back(frame_id);
    return true;
}

void BufferPoolManager::flush_all_pages(int fd) {
    // flush_all_pages 的作用：把某个文件在缓冲池中的所有页面都写回磁盘。
    // 参数 fd 用来筛选“属于哪个文件”的页面。
    std::scoped_lock lock{latch_};

    // 遍历页表，找出所有属于指定文件 fd 的页面。
    // 这里不调用 flush_page，是因为当前已经持有 latch_，
    // flush_page 内部也会加同一把锁，直接调用会造成死锁。
    for (const auto& entry : page_table_) {
        const PageId& page_id = entry.first;
        if (page_id.fd != fd || page_id.page_no == INVALID_PAGE_ID) {
            continue;
        }

        // 实验要求 flush_all_pages 将该文件在缓冲池中的所有页面写回磁盘。
        Page* page = &pages_[entry.second];
        disk_manager_->write_page(page->id_.fd, page->id_.page_no, page->data_, PAGE_SIZE);
        page->is_dirty_ = false;
    }
}

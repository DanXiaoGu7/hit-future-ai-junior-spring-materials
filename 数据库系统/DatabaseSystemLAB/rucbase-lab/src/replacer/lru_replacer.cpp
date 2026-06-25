/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "lru_replacer.h"

/*
 * 本文件实现 LRU 页面替换器。先理解几个词：
 *
 * 1. page：数据库中的磁盘页，是真正的数据页。
 * 2. frame：缓冲池中的内存槽位，一个 frame 可以装一个 page。
 * 3. pin：页面正在被使用，不能被淘汰。
 * 4. unpin：页面暂时没人使用了，可以作为淘汰候选。
 * 5. victim：当缓冲池满了，需要选一个可淘汰 frame 腾位置。
 *
 * LRUReplacer 只管理“可以被淘汰的 frame_id”，不直接管理 page 数据。
 * 真正决定页面何时 pin/unpin 的是 BufferPoolManager。
 *
 * 本实现的两个核心数据结构：
 * - LRUlist_：按 LRU 顺序保存可淘汰 frame。头部最旧，尾部最新。
 * - LRUhash_：保存 frame_id 到链表节点的映射，用来 O(1) 删除指定 frame。
 */

LRUReplacer::LRUReplacer(size_t num_pages) { max_size_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::victim(frame_id_t* frame_id) {
    // victim 的目标：从“可淘汰集合”里选出最应该被淘汰的 frame。
    // LRU 的规则是：谁最久没有被使用，就淘汰谁。

    // LRUReplacer 可能被多个线程同时访问，所以每个公开操作都要先加锁。
    // 这里保护的是 LRUlist_ 和 LRUhash_ 这两个必须同步更新的数据结构。
    std::scoped_lock lock{latch_};

    // LRUlist_ 中只保存 pin_count 已经变成 0、可以被淘汰的 frame。
    // 如果链表为空，说明当前没有任何可淘汰页面，缓冲池管理器需要返回失败。
    if (frame_id == nullptr || LRUlist_.empty()) {
        return false;
    }

    // 本实现约定：
    // - 链表头部是最早进入 replacer 的 frame，即最久没有再次被 pin 的 frame。
    // - 链表尾部是最近一次 unpin 后加入 replacer 的 frame。
    // 因此 victim 时取链表头部，符合 LRU 的“淘汰最久未使用”策略。
    //
    // 举例：依次 unpin 1、2、3，则链表是 [1, 2, 3]。
    // 这时 1 最久没被使用，所以 victim 应该返回 1。
    *frame_id = LRUlist_.front();

    // LRUhash_ 记录 frame_id 到链表节点的映射。
    // 从链表删除 frame 时必须同步删除哈希表项，否则后续 pin/unpin 会看到过期位置。
    LRUhash_.erase(*frame_id);
    LRUlist_.pop_front();
    return true;
}

void LRUReplacer::pin(frame_id_t frame_id) {
    // pin 的目标：把一个 frame 从“可淘汰集合”里移除。
    // 原因：页面一旦被 fetch 或 new_page 返回给上层，就正在被使用，不能淘汰。
    std::scoped_lock lock{latch_};

    // pin 表示页面正在被使用，不能被替换算法选中。
    // 如果该 frame 不在 LRUhash_ 中，说明它本来就不在可淘汰集合里，直接返回即可。
    auto it = LRUhash_.find(frame_id);
    if (it == LRUhash_.end()) {
        return;
    }

    // 如果该 frame 在可淘汰集合中，需要同时从链表和哈希表删除。
    // 通过哈希表保存的迭代器可以 O(1) 定位链表节点。
    LRUlist_.erase(it->second);
    LRUhash_.erase(it);
}

void LRUReplacer::unpin(frame_id_t frame_id) {
    // unpin 的目标：把一个 frame 加入“可淘汰集合”。
    // 但只有当 BufferPoolManager 中该页的 pin_count_ 已经变成 0 时才会调用到这里。
    std::scoped_lock lock{latch_};

    // unpin 只有在页面 pin_count 变为 0 时才会由 BufferPoolManager 调用。
    // 此时 frame 才可以重新进入可淘汰集合。
    //
    // 这里过滤两类情况：
    // 1. frame_id 越界，防止错误输入破坏替换器状态。
    // 2. frame 已经在 LRUhash_ 中，说明之前已经加入过，重复 unpin 不能重复插入。
    if (frame_id < 0 || static_cast<size_t>(frame_id) >= max_size_ || LRUhash_.count(frame_id) != 0) {
        return;
    }

    // 新 unpin 的 frame 放到链表尾部，表示它是“最近变为可淘汰”的页面。
    // 这样后续 victim 会优先淘汰链表头部更旧的页面。
    //
    // 举例：当前链表 [1, 2]，现在 unpin(3)，链表变为 [1, 2, 3]。
    // 下一次 victim 仍然先淘汰 1，而不是刚刚 unpin 的 3。
    LRUlist_.push_back(frame_id);
    auto it = LRUlist_.end();
    --it;
    LRUhash_[frame_id] = it;
}

size_t LRUReplacer::Size() {
    // Size 在并发测试中也可能和 pin/unpin/victim 同时执行，
    // 因此读取链表大小时同样需要加锁。
    std::scoped_lock lock{latch_};
    return LRUlist_.size();
}

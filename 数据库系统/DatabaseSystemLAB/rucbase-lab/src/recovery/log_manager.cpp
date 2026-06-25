/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstring>
#include "log_manager.h"

/**
 * @description: 娣诲姞鏃ュ織璁板綍鍒版棩蹇楃紦鍐插尯涓紝骞惰繑鍥炴棩蹇楄褰曞彿
 * @param {LogRecord*} log_record 瑕佸啓鍏ョ紦鍐插尯鐨勬棩蹇楄褰?
 * @return {lsn_t} 杩斿洖璇ユ棩蹇楃殑鏃ュ織璁板綍鍙?
 */
lsn_t LogManager::add_log_to_buffer(LogRecord* log_record) {
    if (log_record == nullptr) {
        return INVALID_LSN;
    }

    std::scoped_lock lock{latch_};

    // 给当前日志分配递增的 lsn，并把日志序列化追加到内存日志缓冲区。
    log_record->lsn_ = global_lsn_.fetch_add(1);
    if (log_buffer_.is_full(static_cast<int>(log_record->log_tot_len_))) {
        flush_log_to_disk();
    }
    log_record->serialize(log_buffer_.buffer_ + log_buffer_.offset_);
    log_buffer_.offset_ += log_record->log_tot_len_;
    return log_record->lsn_;
}

/**
 * @description: 鎶婃棩蹇楃紦鍐插尯鐨勫唴瀹瑰埛鍒扮鐩樹腑锛岀敱浜庣洰鍓嶅彧璁剧疆浜嗕竴涓紦鍐插尯锛屽洜姝ら渶瑕侀樆濉炲叾浠栨棩蹇楁搷浣?
 */
void LogManager::flush_log_to_disk() {
    if (log_buffer_.offset_ == 0) {
        return;
    }
    disk_manager_->write_log(log_buffer_.buffer_, log_buffer_.offset_);
    persist_lsn_ = global_lsn_.load() - 1;
    memset(log_buffer_.buffer_, 0, sizeof(log_buffer_.buffer_));
    log_buffer_.offset_ = 0;
}


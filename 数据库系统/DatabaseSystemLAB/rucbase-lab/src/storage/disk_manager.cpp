/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/disk_manager.h"

#include <assert.h>    // for assert
#include <string.h>    // for memset
#include <sys/stat.h>  // for stat
#include <unistd.h>    // for lseek

#include "defs.h"

/*
 * 本文件实现 DiskManager，即磁盘管理器。
 *
 * 可以把 DiskManager 理解成数据库和操作系统文件之间的“中间层”：
 * - 上层数据库按 page 读写数据。
 * - 操作系统只认识普通文件和字节偏移。
 * - DiskManager 负责把“第 page_no 页”转换成文件中的字节位置。
 *
 * 先理解几个概念：
 * 1. fd：file descriptor，文件描述符。open 文件后，操作系统返回一个整数 fd。
 * 2. page_no：页号。第 0 页、第 1 页、第 2 页等。
 * 3. PAGE_SIZE：每页大小。本项目中一页是 4096 字节。
 * 4. 文件偏移量：某一页在文件中的起始位置，计算方式是 page_no * PAGE_SIZE。
 *
 * 本类主要维护三类状态：
 * - path2fd_：文件路径 -> fd，用于判断文件是否已经打开、防止重复打开。
 * - fd2path_：fd -> 文件路径，用于判断 fd 是否合法，以及关闭文件时反查路径。
 * - fd2pageno_：fd -> 下一个可分配页号，用于 allocate_page 自增分配页号。
 *
 * 验收时可以抓住一句话：
 * DiskManager 负责“管理文件打开状态，并把页号转换成文件偏移来读写磁盘页”。
 */

// 构造函数初始化每个 fd 对应的已分配页号。
// 初始值为 0，表示每个文件一开始都从第 0 页开始分配。
DiskManager::DiskManager() { memset(fd2pageno_, 0, MAX_FD * (sizeof(std::atomic<page_id_t>) / sizeof(char))); }

/**
 * @description: 将数据写入文件的指定磁盘页面中
 * @param {int} fd 磁盘文件的文件句柄
 * @param {page_id_t} page_no 写入目标页面的page_id
 * @param {char} *offset 要写入磁盘的数据
 * @param {int} num_bytes 要写入磁盘的数据大小
 */
void DiskManager::write_page(int fd, page_id_t page_no, const char *offset, int num_bytes) {
    // write_page 的作用：把内存中的一段数据写入磁盘文件的指定页面。
    //
    // 参数含义：
    // - fd：目标文件的文件描述符，用来确定写入哪个文件。
    // - page_no：目标页号，用来确定写入文件中的哪一页。
    // - offset：内存中待写入数据的起始地址。
    // - num_bytes：本次写入多少字节。

    // fd2path_ 只记录当前由 DiskManager 打开的文件描述符；如果查不到，说明 fd 非法或已经关闭。
    // 对未打开文件写页属于错误操作，所以抛 FileNotOpenError。
    if (!fd2path_.count(fd)) {
        throw FileNotOpenError(fd);
    }

    // 每个页在文件中占 PAGE_SIZE 字节，第 page_no 页的起始偏移量为 page_no * PAGE_SIZE。
    // lseek 将文件读写位置移动到目标页开头，后续 write 才会写入正确位置。
    //
    // 举例：PAGE_SIZE = 4096，第 2 页的起始位置是 2 * 4096 = 8192。
    if (lseek(fd, page_no * PAGE_SIZE, SEEK_SET) == -1) {
        throw UnixError();
    }

    // offset 指向内存中的待写入数据，num_bytes 表示本次写入字节数。
    // 正常情况下 num_bytes 等于 PAGE_SIZE，也可能只写页头等局部内容。
    ssize_t bytes_write = write(fd, offset, num_bytes);

    // write 返回实际写入字节数；若少写，说明页数据没有完整落盘，必须报错。
    if (bytes_write != num_bytes) {
        throw InternalError("DiskManager::write_page Error");
    }
}

/**
 * @description: 读取文件中指定编号的页面中的部分数据到内存中
 * @param {int} fd 磁盘文件的文件句柄
 * @param {page_id_t} page_no 指定的页面编号
 * @param {char} *offset 读取的内容写入到offset中
 * @param {int} num_bytes 读取的数据量大小
 */
void DiskManager::read_page(int fd, page_id_t page_no, char *offset, int num_bytes) {
    // read_page 的作用：从磁盘文件的指定页面读取数据到内存。
    //
    // 参数含义：
    // - fd：目标文件的文件描述符。
    // - page_no：目标页号。
    // - offset：内存接收缓冲区的起始地址。
    // - num_bytes：本次读取多少字节。

    // 读页前先确认 fd 是 DiskManager 当前管理的打开文件，避免使用无效文件描述符。
    if (!fd2path_.count(fd)) {
        throw FileNotOpenError(fd);
    }

    // 根据页号计算该页在文件中的起始偏移量，并移动文件读写位置。
    if (lseek(fd, page_no * PAGE_SIZE, SEEK_SET) == -1) {
        throw UnixError();
    }

    // 将磁盘文件中的 num_bytes 字节读入 offset 指向的内存缓冲区。
    ssize_t bytes_read = read(fd, offset, num_bytes);

    // read 返回实际读取字节数；若不足，说明目标页不存在或文件内容不完整。
    if (bytes_read != num_bytes) {
        throw InternalError("DiskManager::read_page Error");
    }
}

/**
 * @description: 分配一个新的页号
 * @return {page_id_t} 分配的新页号
 * @param {int} fd 指定文件的文件句柄
 */
page_id_t DiskManager::allocate_page(int fd) {
    // 简单的自增分配策略，指定文件的页面编号加1
    //
    // fd2pageno_[fd] 表示该文件下一个可分配的页号。
    // 后置 ++ 的效果是：先返回当前页号，再把计数加 1。
    //
    // 举例：fd2pageno_[fd] 初始是 0：
    // 第一次调用返回 0，然后变成 1；
    // 第二次调用返回 1，然后变成 2。
    assert(fd >= 0 && fd < MAX_FD);
    return fd2pageno_[fd]++;
}

// 当前实验没有真正回收磁盘页，函数保留是为了匹配后续接口。
void DiskManager::deallocate_page(__attribute__((unused)) page_id_t page_id) {}

bool DiskManager::is_dir(const std::string& path) {
    // stat 可以查询路径的文件系统信息。
    // S_ISDIR 用来判断该路径是否是目录。
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

void DiskManager::create_dir(const std::string &path) {
    // Create a subdirectory
    std::string cmd = "mkdir " + path;
    if (system(cmd.c_str()) < 0) {  // 创建一个名为path的目录
        throw UnixError();
    }
}

void DiskManager::destroy_dir(const std::string &path) {
    std::string cmd = "rm -r " + path;
    if (system(cmd.c_str()) < 0) {
        throw UnixError();
    }
}

/**
 * @description: 判断指定路径文件是否存在
 * @return {bool} 若指定路径文件存在则返回true 
 * @param {string} &path 指定路径文件
 */
bool DiskManager::is_file(const std::string &path) {
    // 用struct stat获取文件信息
    // stat 返回 0 表示路径存在；S_ISREG 表示这是普通文件，不是目录。
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

/**
 * @description: 用于创建指定路径文件
 * @return {*}
 * @param {string} &path
 */
void DiskManager::create_file(const std::string &path) {
    // create_file 的作用：在磁盘上创建一个新文件。
    //
    // 注意：它只负责“创建”，不负责“打开并登记到 path2fd_/fd2path_”。
    // 如果创建完就登记为打开状态，会和 open_file 的职责混在一起。

    // create_file 只负责创建新文件；如果文件已存在，按测试要求抛 FileExistsError。
    if (is_file(path)) {
        throw FileExistsError(path);
    }

    // O_CREAT 表示不存在时创建文件，O_EXCL 与 O_CREAT 配合保证创建动作具有排他性。
    // O_RDWR 让创建出的文件可读写；权限 S_IRUSR | S_IWUSR 表示当前用户可读可写。
    int fd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        throw UnixError();
    }

    // 这里只创建文件，不把它登记为“已打开”；因此创建成功后立即关闭临时 fd。
    if (close(fd) < 0) {
        throw UnixError();
    }
}

/**
 * @description: 删除指定路径的文件
 * @param {string} &path 文件所在路径
 */
void DiskManager::destroy_file(const std::string &path) {
    // destroy_file 的作用：删除磁盘上的文件。
    //
    // 删除文件前必须确认两件事：
    // 1. 文件确实存在。
    // 2. 文件没有被当前 DiskManager 打开。
    //
    // 第二点很重要：如果文件还在 path2fd_ 中，说明仍然有人可能通过 fd 读写它。

    // 删除前先确认文件存在；不存在时抛出测试期望的 FileNotFoundError。
    if (!is_file(path)) {
        throw FileNotFoundError(path);
    }

    // 如果文件仍在 path2fd_ 中，说明它还处于打开状态，不能直接 unlink。
    if (path2fd_.count(path)) {
        throw FileNotClosedError(path);
    }

    // unlink 删除目录项；文件未打开时，该调用成功后文件即被删除。
    if (unlink(path.c_str()) < 0) {
        throw UnixError();
    }
}


/**
 * @description: 打开指定路径文件 
 * @return {int} 返回打开的文件的文件句柄
 * @param {string} &path 文件所在路径
 */
int DiskManager::open_file(const std::string &path) {
    // open_file 的作用：以读写方式打开一个已经存在的文件，并记录打开状态。
    //
    // 打开成功后一定要同时维护两个映射：
    // - path2fd_[path] = fd：通过路径知道文件已经打开。
    // - fd2path_[fd] = path：通过 fd 知道它对应哪个文件。

    // open_file 只能打开已经存在的文件；不存在时抛 FileNotFoundError。
    if (!is_file(path)) {
        throw FileNotFoundError(path);
    }

    // path2fd_ 中已有该路径，说明同一个文件已经被打开，不能重复打开。
    if (path2fd_.count(path)) {
        throw FileNotClosedError(path);
    }

    // 使用 O_RDWR 以读写方式打开，满足后续 read_page/write_page 的需求。
    int fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        throw UnixError();
    }

    // 维护双向映射：路径可查 fd，fd 也可查路径，便于后续关闭和合法性检查。
    path2fd_[path] = fd;
    fd2path_[fd] = path;
    return fd;
}

/**
 * @description:用于关闭指定路径文件 
 * @param {int} fd 打开的文件的文件句柄
 */
void DiskManager::close_file(int fd) {
    // close_file 的作用：关闭一个已经打开的文件，并清理打开状态。
    //
    // 和 open_file 对应，关闭成功后必须同时清理 fd2path_ 和 path2fd_。
    // 如果只删一张表，后续可能误判文件状态：
    // - path2fd_ 没删：会误以为文件仍打开，导致不能再次打开或删除。
    // - fd2path_ 没删：会误以为旧 fd 仍合法。

    // fd 不在 fd2path_ 中，说明该文件没有被 DiskManager 打开或已经关闭。
    if (!fd2path_.count(fd)) {
        throw FileNotOpenError(fd);
    }

    // 关闭前先保存路径，close 成功后需要同时删除 path2fd_ 中的反向记录。
    std::string path = fd2path_[fd];
    if (close(fd) < 0) {
        throw UnixError();
    }

    // 文件已经关闭，两个映射表都必须同步清理，避免后续误判为仍处于打开状态。
    fd2path_.erase(fd);
    path2fd_.erase(path);
}


/**
 * @description: 获得文件的大小
 * @return {int} 文件的大小
 * @param {string} &file_name 文件名
 */
int DiskManager::get_file_size(const std::string &file_name) {
    struct stat stat_buf;
    int rc = stat(file_name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

/**
 * @description: 根据文件句柄获得文件名
 * @return {string} 文件句柄对应文件的文件名
 * @param {int} fd 文件句柄
 */
std::string DiskManager::get_file_name(int fd) {
    if (!fd2path_.count(fd)) {
        throw FileNotOpenError(fd);
    }
    return fd2path_[fd];
}

/**
 * @description:  获得文件名对应的文件句柄
 * @return {int} 文件句柄
 * @param {string} &file_name 文件名
 */
int DiskManager::get_file_fd(const std::string &file_name) {
    if (!path2fd_.count(file_name)) {
        return open_file(file_name);
    }
    return path2fd_[file_name];
}


/**
 * @description:  读取日志文件内容
 * @return {int} 返回读取的数据量，若为-1说明读取数据的起始位置超过了文件大小
 * @param {char} *log_data 读取内容到log_data中
 * @param {int} size 读取的数据量大小
 * @param {int} offset 读取的内容在文件中的位置
 */
int DiskManager::read_log(char *log_data, int size, int offset) {
    // read log file from the previous end
    if (log_fd_ == -1) {
        log_fd_ = open_file(LOG_FILE_NAME);
    }
    int file_size = get_file_size(LOG_FILE_NAME);
    if (offset > file_size) {
        return -1;
    }

    size = std::min(size, file_size - offset);
    if(size == 0) return 0;
    lseek(log_fd_, offset, SEEK_SET);
    ssize_t bytes_read = read(log_fd_, log_data, size);
    assert(bytes_read == size);
    return bytes_read;
}


/**
 * @description: 写日志内容
 * @param {char} *log_data 要写入的日志内容
 * @param {int} size 要写入的内容大小
 */
void DiskManager::write_log(char *log_data, int size) {
    if (log_fd_ == -1) {
        log_fd_ = open_file(LOG_FILE_NAME);
    }

    // write from the file_end
    lseek(log_fd_, 0, SEEK_END);
    ssize_t bytes_write = write(log_fd_, log_data, size);
    if (bytes_write != size) {
        throw UnixError();
    }
}

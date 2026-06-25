# DiskManager 实现说明

本文档记录 Lab0 任务二中 `DiskManager` 的代码修改内容。修改文件为：

```text
rucbase-lab/src/storage/disk_manager.cpp
```

本次实现的目标是补全磁盘管理器的文件操作和页读写操作，使任务三中的 `disk_manager_test` 单元测试能够通过。

## 阅读代码部分问题答案

### 1. `PageId::fd` 和 `PageId::page_no` 的含义是什么？

`PageId` 是一个页的唯一标识。由于数据库中可能同时打开多个磁盘文件，单独使用页号无法唯一确定一个页，所以需要同时记录文件描述符和页号。

`PageId::fd` 表示该页所属磁盘文件被打开后得到的文件描述符。它用于区分页面属于哪一个已经打开的文件。

`PageId::page_no` 表示该页在对应磁盘文件中的页号。页号从文件内部定位页面，例如第 0 页、第 1 页、第 2 页等。

因此，一个页面在磁盘上的位置可以由 `(fd, page_no)` 共同确定：

```text
fd       -> 确定是哪一个文件
page_no  -> 确定是该文件中的哪一页
```

在读写页面时，`DiskManager` 会根据 `page_no * PAGE_SIZE` 计算该页在文件中的字节偏移量。

### 2. `path2fd_` 的作用是什么？

`path2fd_` 是从文件路径到文件描述符的映射：

```cpp
std::unordered_map<std::string, int> path2fd_;
```

它的作用是记录当前已经打开的文件。通过文件路径可以快速找到对应的 `fd`。

主要用途包括：

1. 判断某个文件是否已经被打开。
2. 防止同一个文件被重复打开。
3. 删除文件前判断该文件是否仍处于打开状态。
4. 在 `get_file_fd` 中根据文件名获取对应的文件描述符。

例如，如果 `path2fd_` 中已经存在 `"student"`，说明 `student` 文件当前已经打开，再次调用 `open_file("student")` 时应抛出异常。

### 3. `fd2path_` 的作用是什么？

`fd2path_` 是从文件描述符到文件路径的映射：

```cpp
std::unordered_map<int, std::string> fd2path_;
```

它的作用是根据 `fd` 反查对应的文件路径。

主要用途包括：

1. 判断传入的 `fd` 是否是当前 `DiskManager` 管理的合法文件描述符。
2. 在 `close_file(fd)` 时找到该 `fd` 对应的文件路径，便于同时清理 `path2fd_`。
3. 在 `get_file_name(fd)` 中根据文件描述符获取文件名。
4. 在 `read_page` 和 `write_page` 中检查文件是否已经打开。

如果 `fd2path_` 中不存在某个 `fd`，说明这个 `fd` 未打开、已经关闭，或者不是由当前 `DiskManager` 管理。

### 4. `path2fd_` 和 `fd2path_` 的关系是什么？

`path2fd_` 和 `fd2path_` 是一组双向映射。

```text
path2fd_: 文件路径 -> 文件描述符
fd2path_: 文件描述符 -> 文件路径
```

当打开文件成功时，需要同时插入两张表：

```cpp
path2fd_[path] = fd;
fd2path_[fd] = path;
```

当关闭文件成功时，也必须同时删除两张表中的记录：

```cpp
fd2path_.erase(fd);
path2fd_.erase(path);
```

这样可以保证文件打开状态的一致性。如果只更新其中一张表，系统可能会出现错误判断，例如已经关闭的文件仍被认为打开，或者已经打开的文件无法通过 `fd` 反查路径。

### 5. `fd2pageno_` 的作用是什么？

`fd2pageno_` 用于记录每个文件当前已经分配到的页号位置：

```cpp
std::atomic<page_id_t> fd2pageno_[MAX_FD]{};
```

它的下标是文件描述符 `fd`，值表示该文件下一个可分配的页号。

`allocate_page(fd)` 使用它实现简单的自增页号分配：

```cpp
return fd2pageno_[fd]++;
```

例如，某个文件刚开始时 `fd2pageno_[fd] = 0`：

```text
第一次 allocate_page(fd) 返回 0，然后 fd2pageno_[fd] 变为 1
第二次 allocate_page(fd) 返回 1，然后 fd2pageno_[fd] 变为 2
第三次 allocate_page(fd) 返回 2，然后 fd2pageno_[fd] 变为 3
```

因此，`fd2pageno_` 的作用是为每个文件维护独立的页号分配进度，避免不同文件之间的页号分配互相影响。

## 一、涉及的数据结构

`DiskManager` 中与本次实现直接相关的成员变量有：

```cpp
std::unordered_map<std::string, int> path2fd_;
std::unordered_map<int, std::string> fd2path_;
std::atomic<page_id_t> fd2pageno_[MAX_FD]{};
```

`path2fd_` 用于根据文件路径查找已经打开的文件描述符。它可以判断某个文件是否已经被打开，防止同一个文件被重复打开，也可以在删除文件前判断文件是否仍处于打开状态。

`fd2path_` 用于根据文件描述符反查文件路径。它可以判断传入的 `fd` 是否由 `DiskManager` 管理，防止对未打开或已经关闭的文件执行读写和关闭操作。

`fd2pageno_` 用于记录每个文件已经分配到的页号。`allocate_page` 使用自增方式分配新页号，本次没有修改该函数。

## 二、页读写实现

### 1. `write_page`

函数声明：

```cpp
void DiskManager::write_page(int fd, page_id_t page_no, const char *offset, int num_bytes);
```

实现逻辑：

1. 先检查 `fd2path_` 中是否存在该 `fd`。
2. 如果不存在，说明文件没有被当前 `DiskManager` 打开，抛出 `FileNotOpenError`。
3. 根据 `page_no * PAGE_SIZE` 计算目标页在文件中的起始偏移量。
4. 使用 `lseek(fd, page_no * PAGE_SIZE, SEEK_SET)` 将文件读写位置移动到目标页开头。
5. 调用 `write(fd, offset, num_bytes)` 将内存中的数据写入磁盘文件。
6. 如果实际写入字节数不等于 `num_bytes`，抛出 `InternalError`。

核心代码：

```cpp
if (!fd2path_.count(fd)) {
    throw FileNotOpenError(fd);
}
if (lseek(fd, page_no * PAGE_SIZE, SEEK_SET) == -1) {
    throw UnixError();
}
ssize_t bytes_write = write(fd, offset, num_bytes);
if (bytes_write != num_bytes) {
    throw InternalError("DiskManager::write_page Error");
}
```

### 2. `read_page`

函数声明：

```cpp
void DiskManager::read_page(int fd, page_id_t page_no, char *offset, int num_bytes);
```

实现逻辑：

1. 先检查 `fd` 是否是当前打开的合法文件描述符。
2. 根据页号计算偏移量。
3. 使用 `lseek` 定位到目标页。
4. 调用 `read` 将文件内容读入 `offset` 指向的缓冲区。
5. 如果实际读取字节数不等于 `num_bytes`，抛出 `InternalError`。

核心代码：

```cpp
if (!fd2path_.count(fd)) {
    throw FileNotOpenError(fd);
}
if (lseek(fd, page_no * PAGE_SIZE, SEEK_SET) == -1) {
    throw UnixError();
}
ssize_t bytes_read = read(fd, offset, num_bytes);
if (bytes_read != num_bytes) {
    throw InternalError("DiskManager::read_page Error");
}
```

页偏移量计算方式为：

```text
文件偏移量 = 页号 page_no * 每页大小 PAGE_SIZE
```

例如第 0 页偏移量是 `0`，第 1 页偏移量是 `PAGE_SIZE`，第 2 页偏移量是 `2 * PAGE_SIZE`。

## 三、文件操作实现

### 1. `create_file`

函数声明：

```cpp
void DiskManager::create_file(const std::string &path);
```

实现逻辑：

1. 使用 `is_file(path)` 判断文件是否已经存在。
2. 如果文件已经存在，抛出 `FileExistsError`。
3. 使用 `open` 创建文件，参数为 `O_CREAT | O_EXCL | O_RDWR`。
4. 创建成功后立即关闭文件描述符。

核心代码：

```cpp
if (is_file(path)) {
    throw FileExistsError(path);
}
int fd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
if (fd < 0) {
    throw UnixError();
}
if (close(fd) < 0) {
    throw UnixError();
}
```

这里创建成功后没有把文件加入 `path2fd_` 和 `fd2path_`，因为 `create_file` 只负责创建文件，不负责保持文件打开。真正打开文件由 `open_file` 完成。

### 2. `destroy_file`

函数声明：

```cpp
void DiskManager::destroy_file(const std::string &path);
```

实现逻辑：

1. 如果文件不存在，抛出 `FileNotFoundError`。
2. 如果文件还在 `path2fd_` 中，说明文件未关闭，抛出 `FileNotClosedError`。
3. 调用 `unlink(path.c_str())` 删除文件。
4. 如果系统调用失败，抛出 `UnixError`。

核心代码：

```cpp
if (!is_file(path)) {
    throw FileNotFoundError(path);
}
if (path2fd_.count(path)) {
    throw FileNotClosedError(path);
}
if (unlink(path.c_str()) < 0) {
    throw UnixError();
}
```

### 3. `open_file`

函数声明：

```cpp
int DiskManager::open_file(const std::string &path);
```

实现逻辑：

1. 如果文件不存在，抛出 `FileNotFoundError`。
2. 如果 `path2fd_` 中已经存在该路径，说明文件已经打开，抛出 `FileNotClosedError`。
3. 使用 `open(path.c_str(), O_RDWR)` 以读写方式打开文件。
4. 打开成功后更新 `path2fd_` 和 `fd2path_`。
5. 返回文件描述符。

核心代码：

```cpp
if (!is_file(path)) {
    throw FileNotFoundError(path);
}
if (path2fd_.count(path)) {
    throw FileNotClosedError(path);
}
int fd = open(path.c_str(), O_RDWR);
if (fd < 0) {
    throw UnixError();
}
path2fd_[path] = fd;
fd2path_[fd] = path;
return fd;
```

### 4. `close_file`

函数声明：

```cpp
void DiskManager::close_file(int fd);
```

实现逻辑：

1. 如果 `fd2path_` 中不存在该 `fd`，说明文件没有打开或已经关闭，抛出 `FileNotOpenError`。
2. 先通过 `fd2path_` 保存该 `fd` 对应的路径。
3. 调用 `close(fd)` 关闭文件。
4. 关闭成功后，从 `fd2path_` 和 `path2fd_` 中删除对应记录。

核心代码：

```cpp
if (!fd2path_.count(fd)) {
    throw FileNotOpenError(fd);
}
std::string path = fd2path_[fd];
if (close(fd) < 0) {
    throw UnixError();
}
fd2path_.erase(fd);
path2fd_.erase(path);
```

关闭文件时必须同时清理两个映射表，否则后续可能错误地认为文件仍然打开，导致删除文件失败或重复打开判断错误。

## 四、异常处理对应关系

本次实现中使用的异常类型如下：

| 场景 | 异常 |
| --- | --- |
| 文件已经存在但仍调用 `create_file` | `FileExistsError` |
| 文件不存在但调用 `open_file` | `FileNotFoundError` |
| 文件不存在但调用 `destroy_file` | `FileNotFoundError` |
| 文件仍打开时调用 `destroy_file` | `FileNotClosedError` |
| 重复打开同一个文件 | `FileNotClosedError` |
| 关闭未打开的 `fd` | `FileNotOpenError` |
| 对未打开的 `fd` 读写页 | `FileNotOpenError` |
| `open`、`close`、`lseek`、`unlink` 等系统调用失败 | `UnixError` |
| `read` 或 `write` 字节数不符合预期 | `InternalError` |

这些异常类型与 `src/test/storage/disk_manager_test.cpp` 中的测试期望保持一致。

## 五、测试方法

在 SSH 终端中进入项目构建目录：

```bash
cd ~/winshare/DatabaseSystemLAB/rucbase-lab/build
```

编译并运行磁盘管理器单元测试：

```bash
make disk_manager_test
./bin/disk_manager_test
```

通过时输出中会出现：

```text
[  PASSED  ] 2 tests.
```

本次测试覆盖了两个测试点：

```text
DiskManagerTest.FileOperation
DiskManagerTest.PageOperation
```

`FileOperation` 主要测试文件创建、打开、关闭、删除以及相关异常。

`PageOperation` 主要测试页号分配、页写入、页读取以及读写数据一致性。

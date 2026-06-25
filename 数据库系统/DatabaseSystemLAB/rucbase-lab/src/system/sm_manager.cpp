/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sm_manager.h"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>

#include "index/ix.h"
#include "record/rm.h"
#include "record_printer.h"

/*
 * 验收讲解：System Manager 管理数据库级和表级元数据。
 * db_ 保存“有哪些表、表有哪些列、有哪些索引”等逻辑信息；fhs_ 和 ihs_ 保存已经打开的表文件/索引文件句柄。
 * open_db 要把磁盘上的 db.meta 恢复成内存元数据并打开文件句柄；close_db 要反过来写回元数据并关闭句柄；drop_table 要同步删除索引、表文件和元数据。
 *
 * 可以把这个模块理解成“数据库目录管家”：
 * 1. 数据库本身是一个文件夹，文件夹中有 db.meta、日志文件、表文件、索引文件。
 * 2. db.meta 只保存元数据，不保存真正的表记录；真正的记录由 record 模块管理。
 * 3. fhs_ 是 table name -> RmFileHandle，SQL 执行器通过它读写表记录。
 * 4. ihs_ 是 index file name -> IxIndexHandle，后续索引实验会通过它读写 B+ 树索引。
 * 5. open_db / close_db 会改变当前工作目录，所以打开数据库后，相对路径默认都落在该数据库文件夹里。
 */

/**
 * @description: 判断是否为一个文件夹
 * @return {bool} 返回是否为一个文件夹
 * @param {string&} db_name 数据库文件名称，与文件夹同名
 */
bool SmManager::is_dir(const std::string& db_name) {
    struct stat st;
    return stat(db_name.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

/**
 * @description: 创建数据库，所有的数据库相关文件都放在数据库同名文件夹下
 * @param {string&} db_name 数据库名称
 */
void SmManager::create_db(const std::string& db_name) {
    if (is_dir(db_name)) {
        throw DatabaseExistsError(db_name);
    }
    //为数据库创建一个子目录
    std::string cmd = "mkdir " + db_name;
    if (system(cmd.c_str()) < 0) {  // 创建一个名为db_name的目录
        throw UnixError();
    }
    if (chdir(db_name.c_str()) < 0) {  // 进入名为db_name的目录
        throw UnixError();
    }
    //创建系统目录
    DbMeta *new_db = new DbMeta();
    new_db->name_ = db_name;

    // 注意，此处ofstream会在当前目录创建(如果没有此文件先创建)和打开一个名为DB_META_NAME的文件
    std::ofstream ofs(DB_META_NAME);

    // 将new_db中的信息，按照定义好的operator<<操作符，写入到ofs打开的DB_META_NAME文件中
    ofs << *new_db;  // 注意：此处重载了操作符<<

    delete new_db;

    // 创建日志文件
    disk_manager_->create_file(LOG_FILE_NAME);

    // 回到根目录
    if (chdir("..") < 0) {
        throw UnixError();
    }
}

/**
 * @description: 删除数据库，同时需要清空相关文件以及数据库同名文件夹
 * @param {string&} db_name 数据库名称，与文件夹同名
 */
void SmManager::drop_db(const std::string& db_name) {
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    std::string cmd = "rm -r " + db_name;
    if (system(cmd.c_str()) < 0) {
        throw UnixError();
    }
}

/**
 * @description: 打开数据库，找到数据库对应的文件夹，并加载数据库元数据和相关文件
 * @param {string&} db_name 数据库名称，与文件夹同名
 */
void SmManager::open_db(const std::string& db_name) {
    // 数据库在文件系统中表现为一个同名目录。
    // open_db 的第一步就是确认目录存在，并进入该目录，后续表文件、索引文件和 db.meta 都在此目录下访问。
    // 如果目录不存在，说明用户打开了一个从未 create 过的数据库，应该直接报 DatabaseNotFoundError。
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    // chdir 之后，进程当前目录变成数据库目录。
    // 这样 open_file(tab_name) 实际打开的是“数据库目录/表名”这个表文件。
    if (chdir(db_name.c_str()) < 0) {
        throw UnixError();
    }

    // 重新打开数据库前清理内存中的旧句柄和旧元数据，避免上一次打开的数据库状态残留。
    // 这里不是删除磁盘文件，只是把 SmManager 内存状态恢复成空白，准备装载新的 db.meta。
    db_ = DbMeta();
    fhs_.clear();
    ihs_.clear();

    // db.meta 保存 DbMeta 的序列化内容，operator>> 会恢复数据库名、表元数据、索引元数据。
    // 如果把数据库比作一个文件夹，db.meta 就像目录清单：记录有哪些表、每张表有哪些字段、哪些字段建了索引。
    std::ifstream ifs(DB_META_NAME);
    if (!ifs.is_open()) {
        throw FileNotFoundError(DB_META_NAME);
    }
    ifs >> db_;

    // 打开每张表的数据文件，并把表名到 RmFileHandle 的映射记录在 fhs_ 中。
    // SQL 执行器后续通过 fhs_[tab_name] 访问表中的记录。
    // 注意：db_.tabs_ 中只有元数据，不能直接读写记录；真正读写记录必须依赖 rm_manager_->open_file 得到的文件句柄。
    for (auto &entry : db_.tabs_) {
        const std::string &tab_name = entry.first;
        TabMeta &tab = entry.second;
        fhs_.emplace(tab_name, rm_manager_->open_file(tab_name));

        // 如果表上存在索引，也要打开索引文件，并用索引文件名保存到 ihs_ 中。
        // 索引文件名由表名和索引列共同决定，保证同一张表的不同组合索引能对应不同文件。
        // 当前实验主要要求打开已有句柄；create_index/drop_index 是后续实验继续补充的内容。
        for (auto &index : tab.indexes) {
            std::string index_name = ix_manager_->get_index_name(tab_name, index.cols);
            ihs_.emplace(index_name, ix_manager_->open_index(tab_name, index.cols));
        }
    }
}

/**
 * @description: 把数据库相关的元数据刷入磁盘中
 */
void SmManager::flush_meta() {
    // 默认清空文件
    std::ofstream ofs(DB_META_NAME);
    ofs << db_;
}

/**
 * @description: 关闭数据库并把数据落盘
 */
void SmManager::close_db() {
    // 关闭数据库前先把最新元数据写回 db.meta，保证 create/drop table 后的状态能持久保存。
    // 如果不 flush_meta，内存中的 db_ 改动会随着程序退出丢失；下次 open_db 看到的仍是旧表结构。
    flush_meta();

    // 关闭所有索引文件。IxManager::close_index 内部会写回索引文件头并 flush 对应 fd 的脏页。
    // 先遍历 ihs_ 是因为 ihs_ 里保存了所有已经打开的索引句柄；关闭后再 clear，避免留下悬空句柄。
    for (auto &entry : ihs_) {
        ix_manager_->close_index(entry.second.get());
    }
    ihs_.clear();

    // 关闭所有表数据文件。RmManager::close_file 内部会写回 file_hdr 并 flush 对应 fd 的脏页。
    // 表文件头中包含记录长度、页数、空闲页链表等信息，关闭时写回才能保证下次打开继续使用。
    for (auto &entry : fhs_) {
        rm_manager_->close_file(entry.second.get());
    }
    fhs_.clear();

    // open_db 进入了数据库目录；close_db 结束时回到上一级，方便之后打开/删除其他数据库。
    if (chdir("..") < 0) {
        throw UnixError();
    }
}

/**
 * @description: 显示所有的表,通过测试需要将其结果写入到output.txt,详情看题目文档
 * @param {Context*} context 
 */
void SmManager::show_tables(Context* context) {
    std::fstream outfile;
    outfile.open("output.txt", std::ios::out | std::ios::app);
    outfile << "| Tables |\n";
    RecordPrinter printer(1);
    printer.print_separator(context);
    printer.print_record({"Tables"}, context);
    printer.print_separator(context);
    for (auto &entry : db_.tabs_) {
        auto &tab = entry.second;
        printer.print_record({tab.name}, context);
        outfile << "| " << tab.name << " |\n";
    }
    printer.print_separator(context);
    outfile.close();
}

/**
 * @description: 显示表的元数据
 * @param {string&} tab_name 表名称
 * @param {Context*} context 
 */
void SmManager::desc_table(const std::string& tab_name, Context* context) {
    TabMeta &tab = db_.get_table(tab_name);

    std::vector<std::string> captions = {"Field", "Type", "Index"};
    RecordPrinter printer(captions.size());
    // Print header
    printer.print_separator(context);
    printer.print_record(captions, context);
    printer.print_separator(context);
    // Print fields
    for (auto &col : tab.cols) {
        std::vector<std::string> field_info = {col.name, coltype2str(col.type), col.index ? "YES" : "NO"};
        printer.print_record(field_info, context);
    }
    // Print footer
    printer.print_separator(context);
}

/**
 * @description: 创建表
 * @param {string&} tab_name 表的名称
 * @param {vector<ColDef>&} col_defs 表的字段
 * @param {Context*} context 
 */
void SmManager::create_table(const std::string& tab_name, const std::vector<ColDef>& col_defs, Context* context) {
    if (db_.is_table(tab_name)) {
        throw TableExistsError(tab_name);
    }
    // Create table meta
    // curr_offset 表示“当前字段应该从记录的第几个字节开始存”。
    // 例如 int id 占 4 字节，char(20) name 紧跟其后，那么 name.offset 就是 4。
    int curr_offset = 0;
    TabMeta tab;
    tab.name = tab_name;
    for (auto &col_def : col_defs) {
        // ColMeta 是列的元数据，只描述列名、类型、长度、偏移和是否有索引，不保存具体数据。
        ColMeta col = {.tab_name = tab_name,
                       .name = col_def.name,
                       .type = col_def.type,
                       .len = col_def.len,
                       .offset = curr_offset,
                       .index = false};
        // 下一个字段的起始位置 = 当前起始位置 + 当前字段长度。
        // 所有字段长度累加起来，就是一条记录在磁盘页中占用的固定字节数。
        curr_offset += col_def.len;
        tab.cols.push_back(col);
    }
    // Create & open record file
    int record_size = curr_offset;  // record_size 是一条用户记录的总字节数，也就是所有字段 len 累加后的结果。
    // create_file 只创建空表文件并写入表文件头；此时表中还没有任何用户记录。
    rm_manager_->create_file(tab_name, record_size);
    // 把表元数据登记到数据库元数据中，否则 show tables / desc table / open_db 都不知道这张表存在。
    db_.tabs_[tab_name] = tab;
    // fhs_[tab_name] = rm_manager_->open_file(tab_name);
    // 立刻打开表文件，后续 insert/select/delete/update 就能通过 fhs_ 找到它。
    fhs_.emplace(tab_name, rm_manager_->open_file(tab_name));

    // create table 改变了 db_，所以要写回 db.meta。
    flush_meta();
}

/**
 * @description: 删除表
 * @param {string&} tab_name 表的名称
 * @param {Context*} context
 */
void SmManager::drop_table(const std::string& tab_name, Context* context) {
    // 删表前必须先确认表存在，测试中 drop 不存在的表应抛 TableNotFoundError 并输出 failure。
    // 这一步保护后面的 get_table、destroy_file 等操作不会对不存在的文件误操作。
    if (!db_.is_table(tab_name)) {
        throw TableNotFoundError(tab_name);
    }

    // 这里必须用引用。TabMeta 的拷贝构造只复制表名和列信息，不复制 indexes；如果用拷贝，下面就看不到该表已有索引。
    // 换句话说，TabMeta &tab 指向 db_ 中真正那份表元数据，后续读取 indexes 才完整。
    TabMeta &tab = db_.get_table(tab_name);

    // 先关闭并删除该表上的所有索引文件。
    // 文件必须先 close，才能 destroy，否则 DiskManager 会认为文件仍处于打开状态。
    // 删除表时不能只删表文件，因为索引文件依赖这张表；表没了，索引也必须一起清理。
    for (auto &index : tab.indexes) {
        std::string index_name = ix_manager_->get_index_name(tab_name, index.cols);
        auto ih = ihs_.find(index_name);
        if (ih != ihs_.end()) {
            // ihs_ 中存在该索引句柄，说明它当前是打开状态，必须先关闭并从 map 中移除。
            ix_manager_->close_index(ih->second.get());
            ihs_.erase(ih);
        }
        if (ix_manager_->exists(tab_name, index.cols)) {
            // destroy_index 删除磁盘上的索引文件，和上面的 close_index 不是一回事。
            ix_manager_->destroy_index(tab_name, index.cols);
        }
    }

    // 关闭表数据文件并删除磁盘上的表文件。
    auto fh = fhs_.find(tab_name);
    if (fh != fhs_.end()) {
        // 只要表文件在 fhs_ 中，就代表当前数据库打开期间它被打开过；删除前必须关闭。
        rm_manager_->close_file(fh->second.get());
        fhs_.erase(fh);
    }
    // destroy_file 删除真正的表数据文件，里面原本保存所有记录页。
    rm_manager_->destroy_file(tab_name);

    // 最后更新内存中的数据库元数据，并刷回 db.meta。
    // 先删文件再删元数据，是为了保证如果文件操作报错，db_ 中还保留原表信息，便于定位问题。
    db_.tabs_.erase(tab_name);
    flush_meta();
}

/**
 * @description: 创建索引
 * @param {string&} tab_name 表的名称
 * @param {vector<string>&} col_names 索引包含的字段名称
 * @param {Context*} context
 */
void SmManager::create_index(const std::string& tab_name, const std::vector<std::string>& col_names, Context* context) {
    
}

/**
 * @description: 删除索引
 * @param {string&} tab_name 表名称
 * @param {vector<string>&} col_names 索引包含的字段名称
 * @param {Context*} context
 */
void SmManager::drop_index(const std::string& tab_name, const std::vector<std::string>& col_names, Context* context) {
    
}

/**
 * @description: 删除索引
 * @param {string&} tab_name 表名称
 * @param {vector<ColMeta>&} 索引包含的字段元数据
 * @param {Context*} context
 */
void SmManager::drop_index(const std::string& tab_name, const std::vector<ColMeta>& cols, Context* context) {
    
}





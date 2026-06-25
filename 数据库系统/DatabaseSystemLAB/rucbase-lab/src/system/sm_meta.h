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

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "errors.h"
#include "sm_defs.h"

/* 瀛楁鍏冩暟鎹?*/
struct ColMeta {
    std::string tab_name;   // 瀛楁鎵€灞炶〃鍚嶇О
    std::string name;       // 瀛楁鍚嶇О
    ColType type;           // 瀛楁绫诲瀷
    int len;                // 瀛楁闀垮害
    int offset;             // 瀛楁浣嶄簬璁板綍涓殑鍋忕Щ閲?
    bool index;             /** unused */

    friend std::ostream &operator<<(std::ostream &os, const ColMeta &col) {
        // ColMeta涓湁鍚勪釜鍩烘湰绫诲瀷鐨勫彉閲忥紝鐒跺悗璋冪敤閲嶈浇鐨勮繖浜涘彉閲忕殑鎿嶄綔绗?<锛堝叿浣撳疄鐜伴€昏緫鍦╠efs.h锛?
        return os << col.tab_name << ' ' << col.name << ' ' << col.type << ' ' << col.len << ' ' << col.offset << ' '
                  << col.index;
    }

    friend std::istream &operator>>(std::istream &is, ColMeta &col) {
        return is >> col.tab_name >> col.name >> col.type >> col.len >> col.offset >> col.index;
    }
};

/* 绱㈠紩鍏冩暟鎹?*/
struct IndexMeta {
    std::string tab_name;           // 绱㈠紩鎵€灞炶〃鍚嶇О
    int col_tot_len;                // 绱㈠紩瀛楁闀垮害鎬诲拰
    int col_num;                    // 绱㈠紩瀛楁鏁伴噺
    std::vector<ColMeta> cols;      // 绱㈠紩鍖呭惈鐨勫瓧娈?

    friend std::ostream &operator<<(std::ostream &os, const IndexMeta &index) {
        os << index.tab_name << " " << index.col_tot_len << " " << index.col_num;
        for(auto& col: index.cols) {
            os << "\n" << col;
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, IndexMeta &index) {
        is >> index.tab_name >> index.col_tot_len >> index.col_num;
        for(int i = 0; i < index.col_num; ++i) {
            ColMeta col;
            is >> col;
            index.cols.push_back(col);
        }
        return is;
    }
};

/* 琛ㄥ厓鏁版嵁 */
struct TabMeta {
    std::string name;                   // 琛ㄥ悕绉?
    std::vector<ColMeta> cols;          // 琛ㄥ寘鍚殑瀛楁
    std::vector<IndexMeta> indexes;     // 琛ㄤ笂寤虹珛鐨勭储寮?

    TabMeta(){}

    TabMeta(const TabMeta &other) {
        name = other.name;
        for(auto col : other.cols) cols.push_back(col);
    }

    /* 鍒ゆ柇褰撳墠琛ㄤ腑鏄惁瀛樺湪鍚嶄负col_name鐨勫瓧娈?*/
    bool is_col(const std::string &col_name) const {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const ColMeta &col) { return col.name == col_name; });
        return pos != cols.end();
    }

    /* 鍒ゆ柇褰撳墠琛ㄤ笂鏄惁寤烘湁鎸囧畾绱㈠紩锛岀储寮曞寘鍚殑瀛楁涓篶ol_names */
    bool is_index(const std::vector<std::string>& col_names) const {
        for(auto& index: indexes) {
            const auto index_col_num = static_cast<size_t>(index.col_num);
            if(index_col_num == col_names.size()) {
                size_t i = 0;
                for(; i < index_col_num; ++i) {
                    if(index.cols[i].name.compare(col_names[i]) != 0)
                        break;
                }
                if(i == index_col_num) return true;
            }
        }

        return false;
    }

    /* 鏍规嵁瀛楁鍚嶇О闆嗗悎鑾峰彇绱㈠紩鍏冩暟鎹?*/
    std::vector<IndexMeta>::iterator get_index_meta(const std::vector<std::string>& col_names) {
        for(auto index = indexes.begin(); index != indexes.end(); ++index) {
            if(static_cast<size_t>((*index).col_num) != col_names.size()) continue;
            auto& index_cols = (*index).cols;
            size_t i = 0;
            for(; i < col_names.size(); ++i) {
                if(index_cols[i].name.compare(col_names[i]) != 0) 
                    break;
            }
            if(i == col_names.size()) return index;
        }
        throw IndexNotFoundError(name, col_names);
    }

    /* 鏍规嵁瀛楁鍚嶇О鑾峰彇瀛楁鍏冩暟鎹?*/
    std::vector<ColMeta>::iterator get_col(const std::string &col_name) {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const ColMeta &col) { return col.name == col_name; });
        if (pos == cols.end()) {
            throw ColumnNotFoundError(col_name);
        }
        return pos;
    }

    friend std::ostream &operator<<(std::ostream &os, const TabMeta &tab) {
        os << tab.name << '\n' << tab.cols.size() << '\n';
        for (auto &col : tab.cols) {
            os << col << '\n';  // col鏄疌olMeta绫诲瀷锛岀劧鍚庤皟鐢ㄩ噸杞界殑ColMeta鐨勬搷浣滅<<
        }
        os << tab.indexes.size() << "\n";
        for (auto &index : tab.indexes) {
            os << index << "\n";
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, TabMeta &tab) {
        size_t n;
        is >> tab.name >> n;
        for (size_t i = 0; i < n; i++) {
            ColMeta col;
            is >> col;
            tab.cols.push_back(col);
        }
        is >> n;
        for(size_t i = 0; i < n; ++i) {
            IndexMeta index;
            is >> index;
            tab.indexes.push_back(index);
        }
        return is;
    }
};

// 娉ㄦ剰閲嶈浇浜嗘搷浣滅 << 鍜?>>锛岃繖闇€瑕佹洿搴曞眰鍚屾牱閲嶈浇TabMeta銆丆olMeta鐨勬搷浣滅 << 鍜?>>
/* 鏁版嵁搴撳厓鏁版嵁 */
class DbMeta {
    friend class SmManager;

   private:
    std::string name_;                      // 鏁版嵁搴撳悕绉?
    std::map<std::string, TabMeta> tabs_;   // 鏁版嵁搴撲腑鍖呭惈鐨勮〃

   public:
    // DbMeta(std::string name) : name_(name) {}

    /* 鍒ゆ柇鏁版嵁搴撲腑鏄惁瀛樺湪鎸囧畾鍚嶇О鐨勮〃 */
    bool is_table(const std::string &tab_name) const { return tabs_.find(tab_name) != tabs_.end(); }

    void SetTabMeta(const std::string &tab_name, const TabMeta &meta) {
        tabs_[tab_name] = meta;
    }

    /* 鑾峰彇鎸囧畾鍚嶇О琛ㄧ殑鍏冩暟鎹?*/
    TabMeta &get_table(const std::string &tab_name) {
        auto pos = tabs_.find(tab_name);
        if (pos == tabs_.end()) {
            throw TableNotFoundError(tab_name);
        }

        return pos->second;
    }

    // 閲嶈浇鎿嶄綔绗?<<
    friend std::ostream &operator<<(std::ostream &os, const DbMeta &db_meta) {
        os << db_meta.name_ << '\n' << db_meta.tabs_.size() << '\n';
        for (auto &entry : db_meta.tabs_) {
            os << entry.second << '\n';
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, DbMeta &db_meta) {
        size_t n;
        is >> db_meta.name_ >> n;
        for (size_t i = 0; i < n; i++) {
            TabMeta tab;
            is >> tab;
            db_meta.tabs_[tab.name] = tab;
        }
        return is;
    }
};


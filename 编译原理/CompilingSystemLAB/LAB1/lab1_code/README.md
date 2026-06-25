

### 源码与构建文件

- `lexical.l`：词法分析器规则文件
- `syntax.y`：语法分析器规则文件
- `treenode.h`：语法树结构声明
- `treenode.c`：语法树相关函数实现
- `main.c`：主程序入口
- `Makefile`：编译脚本
- `README.md`：当前说明文档

### 编译生成文件

- `lex.yy.c`：由 `flex lexical.l` 生成的词法分析 C 文件
- `syntax.tab.c`：由 `bison -d syntax.y` 生成的语法分析 C 文件
- `syntax.tab.h`：由 `bison -d syntax.y` 生成的头文件
- `parser`：最终可执行程序

### 测试文件

- `testcase_1`：对应指导书必做样例 1.1
- `testcase_2`：对应指导书必做样例 1.2
- `testcase_3`：对应指导书必做样例 1.3
- `testcase_4`：对应指导书必做样例 1.4


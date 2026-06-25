# 实验三：中间代码生成

本目录是实验三代码目录。程序在词法分析、语法分析和语义分析通过后，将 C-- 源程序翻译为三地址形式的中间代码，并输出到指定 .ir 文件。

## 编译环境

建议在 Linux 环境下编译运行，依赖：

- gcc
- flex
- bison
- make

## 编译

```bash
cd lab3_code
make
```

生成的可执行文件为 parser。

## 运行

实验三要求传入输入文件和输出文件两个参数：

```bash
./parser testcases/testcase_1 testcases/out1.ir
```

批量运行指导书必做部分的 2 个测试样例：

```bash
make test
```

清理生成文件：

```bash
make clean
```

## 主要文件

- main.c：主程序入口，完成输入文件读取、语法分析、语义分析和中间代码输出调度。
- intercode.h：中间代码数据结构和翻译函数声明。
- intercode.c：中间代码生成、打印、条件翻译、函数调用翻译和简单优化。
- semantic.c / semantic.h：语义分析和符号表支持。
- syntax.y / lexical.l：语法分析和词法分析。

## 已覆盖功能

- 普通表达式、赋值表达式、算术表达式。
- if、if-else、while、return。
- 函数定义、函数参数、函数调用。
- read/write 特殊输入输出函数。
- 中间代码输出到文件，每行一条三地址码。

## 测试样例说明

- testcases/testcase_1 对应指导书必做样例 3.1：读入整数并输出符号函数结果。
- testcases/testcase_2 对应指导书必做样例 3.2：递归阶乘函数调用。
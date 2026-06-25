# files
- lexical.l  flex源代码
- lexical2.l  引用了syntax.tab.h之后的flex源代码
- syntax.y  bison源代码
- treenode.h  语法分析树结构
- main.c  生成scanner对应的主函数 和lexical.l一起编译
- main2.c  生成parser对应的主函数 和syntax.tab.c一起编译
- main_dbg.c 用于调试
- scanner  词法分析工具
- parser  语法分析工具
- lex.yy.c syntax.tab.c syntax.tab.h  编译生成的中间文件
- testcase_* 测试文件
# 编译
```bash
make clean #清除parser以及所有编译生成的中间文件
make       #编译生成语法分析器parser
make debug #生成具有调试功能（显示状态机）的语法分析器parser
```
# 测试
```bash
./parser testcase_*
```
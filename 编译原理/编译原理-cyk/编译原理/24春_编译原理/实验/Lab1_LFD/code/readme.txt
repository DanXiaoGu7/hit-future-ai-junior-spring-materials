lexical.l  flex源代码
lexical2.l  引用了syntax.tab.h之后的flex源代码
syntax.y  bison源代码
treenode.h  语法分析树结构
main.c  生成scanner对应的主函数 和lexical.l一起编译
main2.c  生成parser对应的主函数 和syntax.tab.c一起编译
scanner  词法分析工具
parser  语法分析工具
lex.yy.c syntax.tab.c syntax.tab.h  编译生成的中间文件
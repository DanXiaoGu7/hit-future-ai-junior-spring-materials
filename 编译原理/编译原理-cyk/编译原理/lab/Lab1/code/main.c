/*用于生成scanner*/
#include<stdio.h> // 包含标准输入输出库
#include<string.h> // 包含字符串处理库
#include<stdlib.h> // 包含标准库函数
#include<math.h> // 包含数学库函数
//#include"treenode.h" // 包含自定义的树节点头文件

extern FILE* yyin; // 声明外部输入文件流指针
//extern Treenode root; // 声明外部树节点root
void yyrestart(FILE *input_file); // 声明重新设置输入文件函数
int yyparse(void); // 声明语法分析函数
int yylex(); // 声明词法分析函数
int hasFault = 0; // 是否存在错误标志，初始化为0

int main(int argc, char** argv)
{
	if (argc > 1) // 判断命令行参数数量是否大于1
	{
		if (!(yyin = fopen(argv[1], "r"))) // 打开命令行参数指定的文件
		{
			perror(argv[1]); // 输出错误信息
			return 1; // 返回错误码1
		}
	}
	while (yylex() != 0); // 循环调用词法分析函数，直到返回0
	return 0; // 返回0表示正常退出
}

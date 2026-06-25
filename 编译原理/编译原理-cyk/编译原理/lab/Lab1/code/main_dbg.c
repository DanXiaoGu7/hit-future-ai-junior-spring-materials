/*用于生成parser*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include"treenode.h"

extern FILE* yyin;
extern Treenode root;
extern int yydebug;

void yyrestart(FILE *input_file);
int yyparse(void);
int yylex();
int hasFault=0;

int main(int argc,char** argv)
{
	if (argc<=1) return 1;
    	FILE* f=fopen(argv[1], "r");
    	if (!f)
    	{
        	perror(argv[1]);
        	return 1;
    	}
    	yyrestart(f);
		yydebug = 1;
    	yyparse();
    	if (!hasFault)
    		dfs(root,0);
    	return 0;
}

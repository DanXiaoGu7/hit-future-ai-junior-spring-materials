#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include"treenode.h"

extern FILE* yyin;
extern Treenode root;
void yyrestart(FILE *input_file);
int yyparse(void);
int yylex();
int hasFault=0;

int main(int argc,char** argv)
{
	if(argc>1)
	{
		if(!(yyin=fopen(argv[1],"r")))
		{
			perror(argv[1]);
			return 1;
		}
	}
	while(yylex()!=0);
	return 0;
}

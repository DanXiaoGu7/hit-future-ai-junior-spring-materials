#include <stdlib.h>
#include <stdio.h>
#include "treenode.h"
#include "semantics.h"
extern FILE* yyin;
extern Treenode root;
void yyrestart (FILE *input_file);
int yyparse(void);
int hasFault = 0;
int main(int argc, char** argv)
{
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (!f){
        perror(argv[1]);  
        return 1;
    }
    yyrestart(f); 
    yyparse();
    if (!hasFault){
        //dfs(root, 0);
        semantic_analysis(root);
    }
    return 0;
}



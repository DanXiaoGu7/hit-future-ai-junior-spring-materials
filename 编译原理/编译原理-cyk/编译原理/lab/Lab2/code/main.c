#include "semantic.h"
#include "syntax.tab.h"

extern pNode root;

extern int yylineno;
extern int yyparse();
extern void yyrestart(FILE*);

unsigned lexError = FALSE;
unsigned synError = FALSE;

int main(int argc, char** argv) {
    if (argc <= 1) {
        yyparse();
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }

    yyrestart(f);
    yyparse();
    if (!lexError && !synError) {
        table = initTable();//初始化符号表
        //printTreeInfo(root, 0);
        traverseTree(root);//从根节点开始语义分析
        //printTable(table);
        deleteTable(table);
    }
    delNode(&root);
    return 0;
}

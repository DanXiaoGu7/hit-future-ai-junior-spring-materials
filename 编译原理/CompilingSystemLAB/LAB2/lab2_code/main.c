#include <stdio.h>

#include "semantic.h"
#include "treenode.h"

extern FILE *yyin;
extern TreeNode root;

void yyrestart(FILE *input_file);
int yyparse(void);

int has_fault = 0;

int main(int argc, char **argv) {
    if (argc <= 1) {
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return 1;
    }

    yyrestart(input);
    yyparse();
    fclose(input);

    if (!has_fault) {
        semantic_analyze(root);
    }

    return 0;
}

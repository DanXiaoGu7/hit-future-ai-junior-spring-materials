#ifndef TREENODE_H
#define TREENODE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *yytext;

typedef struct ParseTree {
    int line;
    const char *name;
    int is_leaf;
    union {
        char *text;
        int int_value;
        float float_value;
    } value;
    struct ParseTree *first_child;
    struct ParseTree *next_sibling;
} ParseTree;

typedef ParseTree *TreeNode;

TreeNode new_empty_node(void);
TreeNode new_node(int line, const char *name, int child_count, ...);
void print_tree(TreeNode node, int depth);

#endif

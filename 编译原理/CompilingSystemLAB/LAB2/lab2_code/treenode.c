#include "treenode.h"

TreeNode new_empty_node(void) {
    TreeNode node = (TreeNode)malloc(sizeof(ParseTree));
    if (node == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    node->line = 0;
    node->name = "EMPTY";
    node->is_leaf = 0;
    node->first_child = NULL;
    node->next_sibling = NULL;
    return node;
}

TreeNode new_node(int line, const char *name, int child_count, ...) {
    TreeNode node = (TreeNode)malloc(sizeof(ParseTree));
    if (node == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    node->line = line;
    node->name = name;
    node->is_leaf = (child_count == 0);
    node->first_child = NULL;
    node->next_sibling = NULL;

    if (child_count == 0) {
        if (strcmp(name, "ID") == 0 || strcmp(name, "TYPE") == 0) {
            size_t len = strlen(yytext);
            node->value.text = (char *)malloc(len + 1);
            if (node->value.text == NULL) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            memcpy(node->value.text, yytext, len + 1);
        } else if (strcmp(name, "INT") == 0) {
            node->value.int_value = (int)strtol(yytext, NULL, 0);
        } else if (strcmp(name, "FLOAT") == 0) {
            node->value.float_value = strtof(yytext, NULL);
        }
        return node;
    }

    va_list ap;
    va_start(ap, child_count);

    node->first_child = va_arg(ap, TreeNode);
    TreeNode current = node->first_child;

    for (int i = 1; i < child_count; ++i) {
        current->next_sibling = va_arg(ap, TreeNode);
        current = current->next_sibling;
    }

    current->next_sibling = NULL;
    va_end(ap);

    return node;
}

void print_tree(TreeNode node, int depth) {
    if (node == NULL || node->name == NULL) {
        return;
    }

    if (strcmp(node->name, "EMPTY") != 0) {
        printf("%*s%s", depth * 2, "", node->name);

        if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0) {
            printf(": %s", node->value.text);
        } else if (strcmp(node->name, "INT") == 0) {
            printf(": %d", node->value.int_value);
        } else if (strcmp(node->name, "FLOAT") == 0) {
            printf(": %f", node->value.float_value);
        } else if (!node->is_leaf) {
            printf(" (%d)", node->line);
        }
        printf("\n");
    }

    print_tree(node->first_child, depth + 1);
    print_tree(node->next_sibling, depth);
}

#include "semantic.h"

#include <stdarg.h>

extern int has_fault;

typedef enum {
    BASIC_INT,
    BASIC_FLOAT
} BasicKind;

typedef enum {
    TYPE_BASIC,
    TYPE_ARRAY,
    TYPE_STRUCT
} TypeKind;

typedef struct Type Type;
typedef struct Field Field;
typedef struct VarSymbol VarSymbol;
typedef struct FuncSymbol FuncSymbol;
typedef struct StructSymbol StructSymbol;

struct Type {
    TypeKind kind;
    union {
        BasicKind basic;
        struct {
            Type *elem;
            int size;
        } array;
        struct {
            char *name;
            Field *fields;
        } structure;
    } u;
};

struct Field {
    char *name;
    Type *type;
    Field *next;
};

struct VarSymbol {
    char *name;
    Type *type;
    VarSymbol *next;
};

struct FuncSymbol {
    char *name;
    Type *return_type;
    Field *params;
    int param_count;
    FuncSymbol *next;
};

struct StructSymbol {
    char *name;
    Field *fields;
    StructSymbol *next;
};

static VarSymbol *variables = NULL;
static FuncSymbol *functions = NULL;
static StructSymbol *structures = NULL;

static int last_semantic_error_line = 0;
static int anonymous_struct_count = 0;

static void analyze_ext_def(TreeNode node);
static Type *analyze_specifier(TreeNode node);
static Type *analyze_struct_specifier(TreeNode node);
static void analyze_ext_dec_list(TreeNode node, Type *specifier);
static void analyze_fun_dec(TreeNode node, Type *return_type, int insert_params);
static Field *analyze_var_list(TreeNode node, int insert_symbols, int *param_count);
static Field *analyze_param_dec(TreeNode node, int insert_symbol);
static void analyze_comp_st(TreeNode node, Type *return_type);
static void analyze_stmt_list(TreeNode node, Type *return_type);
static void analyze_stmt(TreeNode node, Type *return_type);
static void analyze_def_list(TreeNode node, Field **struct_fields);
static void analyze_def(TreeNode node, Field **struct_fields);
static void analyze_dec_list(TreeNode node, Type *specifier, Field **struct_fields);
static void analyze_dec(TreeNode node, Type *specifier, Field **struct_fields);
static Type *analyze_exp(TreeNode node);
static int analyze_args(TreeNode node, FuncSymbol *func);

static int is_node(TreeNode node, const char *name) {
    return node != NULL && node->name != NULL && strcmp(node->name, name) == 0;
}

static int is_empty(TreeNode node) {
    return node == NULL || is_node(node, "EMPTY");
}

static TreeNode child(TreeNode node, int index) {
    TreeNode cur = node == NULL ? NULL : node->first_child;
    while (cur != NULL && index > 0) {
        cur = cur->next_sibling;
        --index;
    }
    return cur;
}

static char *xstrdup(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (dst == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    memcpy(dst, src, len + 1);
    return dst;
}

static void report_error(int type, int line, const char *fmt, ...) {
    if (line <= 0 || last_semantic_error_line == line) {
        return;
    }

    last_semantic_error_line = line;
    has_fault = 1;
    printf("Error type %d at Line %d: ", type, line);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    printf(".\n");
}

static Type *new_basic_type(BasicKind basic) {
    Type *type = (Type *)malloc(sizeof(Type));
    if (type == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    type->kind = TYPE_BASIC;
    type->u.basic = basic;
    return type;
}

static Type *new_array_type(Type *elem, int size) {
    Type *type = (Type *)malloc(sizeof(Type));
    if (type == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    type->kind = TYPE_ARRAY;
    type->u.array.elem = elem;
    type->u.array.size = size;
    return type;
}

static Type *new_struct_type(const char *name, Field *fields) {
    Type *type = (Type *)malloc(sizeof(Type));
    if (type == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    type->kind = TYPE_STRUCT;
    type->u.structure.name = xstrdup(name);
    type->u.structure.fields = fields;
    return type;
}

static Field *new_field(const char *name, Type *type) {
    Field *field = (Field *)malloc(sizeof(Field));
    if (field == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    field->name = xstrdup(name);
    field->type = type;
    field->next = NULL;
    return field;
}

static Type *copy_type(Type *type);

static Field *copy_fields(Field *field) {
    Field *head = NULL;
    Field **tail = &head;

    while (field != NULL) {
        *tail = new_field(field->name, copy_type(field->type));
        tail = &(*tail)->next;
        field = field->next;
    }

    return head;
}

static Type *copy_type(Type *type) {
    if (type == NULL) {
        return NULL;
    }

    switch (type->kind) {
    case TYPE_BASIC:
        return new_basic_type(type->u.basic);
    case TYPE_ARRAY:
        return new_array_type(copy_type(type->u.array.elem), type->u.array.size);
    case TYPE_STRUCT:
        return new_struct_type(type->u.structure.name, copy_fields(type->u.structure.fields));
    }

    return NULL;
}

static void free_type(Type *type);

static void free_fields(Field *field) {
    while (field != NULL) {
        Field *next = field->next;
        free(field->name);
        free_type(field->type);
        free(field);
        field = next;
    }
}

static void free_type(Type *type) {
    if (type == NULL) {
        return;
    }

    if (type->kind == TYPE_ARRAY) {
        free_type(type->u.array.elem);
    } else if (type->kind == TYPE_STRUCT) {
        free(type->u.structure.name);
        free_fields(type->u.structure.fields);
    }
    free(type);
}

static int same_type(Type *left, Type *right) {
    if (left == NULL || right == NULL) {
        return 1;
    }
    if (left->kind != right->kind) {
        return 0;
    }

    switch (left->kind) {
    case TYPE_BASIC:
        return left->u.basic == right->u.basic;
    case TYPE_ARRAY:
        return same_type(left->u.array.elem, right->u.array.elem);
    case TYPE_STRUCT:
        if (left->u.structure.name == NULL || right->u.structure.name == NULL) {
            return 1;
        }
        return strcmp(left->u.structure.name, right->u.structure.name) == 0;
    }

    return 0;
}

static int is_int_type(Type *type) {
    return type != NULL && type->kind == TYPE_BASIC && type->u.basic == BASIC_INT;
}

static int is_numeric_type(Type *type) {
    return type != NULL && type->kind == TYPE_BASIC;
}

static VarSymbol *find_variable(const char *name) {
    VarSymbol *cur = variables;
    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static FuncSymbol *find_function(const char *name) {
    FuncSymbol *cur = functions;
    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static StructSymbol *find_structure(const char *name) {
    StructSymbol *cur = structures;
    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static void add_variable(const char *name, Type *type, int line) {
    if (find_variable(name) != NULL || find_structure(name) != NULL) {
        report_error(3, line, "Redefined variable \"%s\"", name);
        return;
    }

    VarSymbol *var = (VarSymbol *)malloc(sizeof(VarSymbol));
    if (var == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    var->name = xstrdup(name);
    var->type = copy_type(type);
    var->next = variables;
    variables = var;
}

static void add_function(const char *name, Type *return_type, Field *params,
                         int param_count, int line) {
    if (find_function(name) != NULL) {
        report_error(4, line, "Redefined function \"%s\"", name);
        return;
    }

    FuncSymbol *func = (FuncSymbol *)malloc(sizeof(FuncSymbol));
    if (func == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    func->name = xstrdup(name);
    func->return_type = copy_type(return_type);
    func->params = copy_fields(params);
    func->param_count = param_count;
    func->next = functions;
    functions = func;
}

static void add_structure(const char *name, Field *fields, int line) {
    if (name == NULL || name[0] == '\0') {
        return;
    }
    if (find_structure(name) != NULL || find_variable(name) != NULL) {
        report_error(16, line, "Duplicated name \"%s\"", name);
        return;
    }

    StructSymbol *structure = (StructSymbol *)malloc(sizeof(StructSymbol));
    if (structure == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    structure->name = xstrdup(name);
    structure->fields = copy_fields(fields);
    structure->next = structures;
    structures = structure;
}

static void free_symbols(void) {
    while (variables != NULL) {
        VarSymbol *next = variables->next;
        free(variables->name);
        free_type(variables->type);
        free(variables);
        variables = next;
    }

    while (functions != NULL) {
        FuncSymbol *next = functions->next;
        free(functions->name);
        free_type(functions->return_type);
        free_fields(functions->params);
        free(functions);
        functions = next;
    }

    while (structures != NULL) {
        StructSymbol *next = structures->next;
        free(structures->name);
        free_fields(structures->fields);
        free(structures);
        structures = next;
    }
}

static TreeNode vardec_id(TreeNode node) {
    TreeNode cur = node;
    while (cur != NULL && !is_empty(cur)) {
        TreeNode first = child(cur, 0);
        if (is_node(first, "ID")) {
            return first;
        }
        cur = first;
    }
    return NULL;
}

static Type *append_array_dimension(Type *type, int size) {
    if (type == NULL || type->kind != TYPE_ARRAY) {
        return new_array_type(type, size);
    }
    type->u.array.elem = append_array_dimension(type->u.array.elem, size);
    return type;
}

static Type *vardec_type(TreeNode node, Type *base) {
    TreeNode first = child(node, 0);
    if (is_node(first, "ID")) {
        return copy_type(base);
    }

    Type *type = vardec_type(first, base);
    TreeNode int_node = child(node, 2);
    return append_array_dimension(type, int_node == NULL ? 0 : int_node->value.int_value);
}

static Field *find_field(Field *fields, const char *name) {
    while (fields != NULL) {
        if (strcmp(fields->name, name) == 0) {
            return fields;
        }
        fields = fields->next;
    }
    return NULL;
}

static void append_field(Field **fields, Field *field) {
    if (field == NULL) {
        return;
    }

    if (*fields == NULL) {
        *fields = field;
        return;
    }

    Field *tail = *fields;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = field;
}

static int is_lvalue(TreeNode exp) {
    TreeNode first = child(exp, 0);
    TreeNode second = first == NULL ? NULL : first->next_sibling;

    if (is_node(first, "ID") && second == NULL) {
        return 1;
    }
    if (is_node(first, "Exp") && (is_node(second, "LB") || is_node(second, "DOT"))) {
        return 1;
    }
    return 0;
}

void semantic_analyze(TreeNode root) {
    variables = NULL;
    functions = NULL;
    structures = NULL;
    last_semantic_error_line = 0;
    anonymous_struct_count = 0;

    if (is_empty(root)) {
        return;
    }

    TreeNode ext_def_list = child(root, 0);
    while (!is_empty(ext_def_list)) {
        analyze_ext_def(child(ext_def_list, 0));
        ext_def_list = child(ext_def_list, 1);
    }

    free_symbols();
}

static void analyze_ext_def(TreeNode node) {
    if (is_empty(node)) {
        return;
    }

    Type *specifier = analyze_specifier(child(node, 0));
    TreeNode second = child(node, 1);

    if (is_node(second, "ExtDecList")) {
        analyze_ext_dec_list(second, specifier);
    } else if (is_node(second, "FunDec")) {
        const char *func_name = child(second, 0)->value.text;
        if (find_function(func_name) != NULL) {
            report_error(4, second->line, "Redefined function \"%s\"", func_name);
        } else {
            analyze_fun_dec(second, specifier, 1);
            analyze_comp_st(child(node, 2), specifier);
        }
    }

    free_type(specifier);
}

static void analyze_ext_dec_list(TreeNode node, Type *specifier) {
    while (!is_empty(node)) {
        TreeNode var_dec = child(node, 0);
        TreeNode id = vardec_id(var_dec);
        Type *type = vardec_type(var_dec, specifier);

        if (id != NULL) {
            add_variable(id->value.text, type, id->line);
        }

        free_type(type);

        if (child(node, 1) == NULL) {
            break;
        }
        node = child(node, 2);
    }
}

static Type *analyze_specifier(TreeNode node) {
    TreeNode first = child(node, 0);
    if (is_node(first, "TYPE")) {
        return strcmp(first->value.text, "int") == 0
                   ? new_basic_type(BASIC_INT)
                   : new_basic_type(BASIC_FLOAT);
    }
    if (is_node(first, "StructSpecifier")) {
        return analyze_struct_specifier(first);
    }
    return NULL;
}

static Type *analyze_struct_specifier(TreeNode node) {
    TreeNode second = child(node, 1);

    if (is_node(second, "Tag")) {
        TreeNode id = child(second, 0);
        StructSymbol *structure = find_structure(id->value.text);
        if (structure == NULL) {
            report_error(17, node->line, "Undefined structure \"%s\"", id->value.text);
            return NULL;
        }
        return new_struct_type(structure->name, copy_fields(structure->fields));
    }

    const char *name = NULL;
    char anonymous_name[32];
    TreeNode def_list = NULL;
    int named_struct = 0;
    if (is_node(second, "OptTag") && !is_empty(child(second, 0))) {
        name = child(second, 0)->value.text;
        def_list = child(node, 3);
        named_struct = 1;
    } else {
        snprintf(anonymous_name, sizeof(anonymous_name), "$anon%d", ++anonymous_struct_count);
        name = anonymous_name;
        def_list = child(node, 3);
    }

    Field *fields = NULL;
    analyze_def_list(def_list, &fields);

    if (named_struct) {
        add_structure(name, fields, node->line);
    }

    Type *result = new_struct_type(name, copy_fields(fields));
    free_fields(fields);
    return result;
}

static void analyze_fun_dec(TreeNode node, Type *return_type, int insert_params) {
    TreeNode id = child(node, 0);
    int param_count = 0;
    Field *params = NULL;

    TreeNode third = child(node, 2);
    if (is_node(third, "VarList")) {
        params = analyze_var_list(third, insert_params, &param_count);
    }

    add_function(id->value.text, return_type, params, param_count, id->line);
    free_fields(params);
}

static Field *analyze_var_list(TreeNode node, int insert_symbols, int *param_count) {
    Field *params = NULL;
    *param_count = 0;

    while (!is_empty(node)) {
        Field *param = analyze_param_dec(child(node, 0), insert_symbols);
        append_field(&params, param);
        ++(*param_count);

        if (child(node, 1) == NULL) {
            break;
        }
        node = child(node, 2);
    }

    return params;
}

static Field *analyze_param_dec(TreeNode node, int insert_symbol) {
    Type *specifier = analyze_specifier(child(node, 0));
    TreeNode var_dec = child(node, 1);
    TreeNode id = vardec_id(var_dec);
    Type *type = vardec_type(var_dec, specifier);
    Field *field = NULL;

    if (id != NULL) {
        field = new_field(id->value.text, copy_type(type));
        if (insert_symbol) {
            add_variable(id->value.text, type, id->line);
        }
    }

    free_type(specifier);
    free_type(type);
    return field;
}

static void analyze_comp_st(TreeNode node, Type *return_type) {
    if (is_empty(node)) {
        return;
    }

    analyze_def_list(child(node, 1), NULL);
    analyze_stmt_list(child(node, 2), return_type);
}

static void analyze_stmt_list(TreeNode node, Type *return_type) {
    while (!is_empty(node)) {
        analyze_stmt(child(node, 0), return_type);
        node = child(node, 1);
    }
}

static void analyze_stmt(TreeNode node, Type *return_type) {
    if (is_empty(node)) {
        return;
    }

    TreeNode first = child(node, 0);

    if (is_node(first, "Exp")) {
        Type *type = analyze_exp(first);
        free_type(type);
    } else if (is_node(first, "CompSt")) {
        analyze_comp_st(first, return_type);
    } else if (is_node(first, "RETURN")) {
        Type *type = analyze_exp(child(node, 1));
        if (!same_type(return_type, type)) {
            report_error(8, node->line, "Type mismatched for return");
        }
        free_type(type);
    } else if (is_node(first, "IF")) {
        Type *cond = analyze_exp(child(node, 2));
        if (cond != NULL && !is_int_type(cond)) {
            report_error(7, child(node, 2)->line, "Type mismatched for operands");
        }
        free_type(cond);
        analyze_stmt(child(node, 4), return_type);
        if (child(node, 5) != NULL) {
            analyze_stmt(child(node, 6), return_type);
        }
    } else if (is_node(first, "WHILE")) {
        Type *cond = analyze_exp(child(node, 2));
        if (cond != NULL && !is_int_type(cond)) {
            report_error(7, child(node, 2)->line, "Type mismatched for operands");
        }
        free_type(cond);
        analyze_stmt(child(node, 4), return_type);
    }
}

static void analyze_def_list(TreeNode node, Field **struct_fields) {
    while (!is_empty(node)) {
        analyze_def(child(node, 0), struct_fields);
        node = child(node, 1);
    }
}

static void analyze_def(TreeNode node, Field **struct_fields) {
    Type *specifier = analyze_specifier(child(node, 0));
    analyze_dec_list(child(node, 1), specifier, struct_fields);
    free_type(specifier);
}

static void analyze_dec_list(TreeNode node, Type *specifier, Field **struct_fields) {
    while (!is_empty(node)) {
        analyze_dec(child(node, 0), specifier, struct_fields);
        if (child(node, 1) == NULL) {
            break;
        }
        node = child(node, 2);
    }
}

static void analyze_dec(TreeNode node, Type *specifier, Field **struct_fields) {
    TreeNode var_dec = child(node, 0);
    TreeNode id = vardec_id(var_dec);
    Type *type = vardec_type(var_dec, specifier);
    int has_initializer = child(node, 1) != NULL;

    if (id == NULL) {
        free_type(type);
        return;
    }

    if (struct_fields != NULL) {
        if (has_initializer) {
            report_error(15, id->line, "Initialized field \"%s\"", id->value.text);
            if (find_field(*struct_fields, id->value.text) == NULL) {
                append_field(struct_fields, new_field(id->value.text, copy_type(type)));
            }
        } else if (find_field(*struct_fields, id->value.text) != NULL) {
            report_error(15, id->line, "Redefined field \"%s\"", id->value.text);
        } else {
            append_field(struct_fields, new_field(id->value.text, copy_type(type)));
        }
        free_type(type);
        return;
    }

    add_variable(id->value.text, type, id->line);

    if (has_initializer) {
        Type *rhs = analyze_exp(child(node, 2));
        if (!same_type(type, rhs)) {
            report_error(5, node->line, "Type mismatched for assignment");
        }
        free_type(rhs);
    }

    free_type(type);
}

static Type *analyze_exp(TreeNode node) {
    TreeNode first = child(node, 0);
    TreeNode second = child(node, 1);

    if (is_node(first, "ID") && second == NULL) {
        VarSymbol *var = find_variable(first->value.text);
        if (var == NULL) {
            report_error(1, first->line, "Undefined variable \"%s\"", first->value.text);
            return NULL;
        }
        return copy_type(var->type);
    }

    if (is_node(first, "INT")) {
        return new_basic_type(BASIC_INT);
    }
    if (is_node(first, "FLOAT")) {
        return new_basic_type(BASIC_FLOAT);
    }

    if (is_node(first, "LP")) {
        return analyze_exp(child(node, 1));
    }

    if (is_node(first, "MINUS")) {
        Type *operand = analyze_exp(child(node, 1));
        Type *result = NULL;
        if (operand != NULL && !is_numeric_type(operand)) {
            report_error(7, node->line, "Type mismatched for operands");
        } else {
            result = copy_type(operand);
        }
        free_type(operand);
        return result;
    }

    if (is_node(first, "NOT")) {
        Type *operand = analyze_exp(child(node, 1));
        Type *result = NULL;
        if (operand != NULL && !is_int_type(operand)) {
            report_error(7, node->line, "Type mismatched for operands");
        } else if (operand != NULL) {
            result = new_basic_type(BASIC_INT);
        }
        free_type(operand);
        return result;
    }

    if (is_node(first, "ID") && is_node(second, "LP")) {
        FuncSymbol *func = find_function(first->value.text);
        if (func == NULL) {
            if (find_variable(first->value.text) != NULL) {
                report_error(11, node->line, "\"%s\" is not a function", first->value.text);
            } else {
                report_error(2, node->line, "Undefined function \"%s\"", first->value.text);
            }
            return NULL;
        }

        TreeNode third = child(node, 2);
        if (is_node(third, "Args")) {
            analyze_args(third, func);
        } else if (func->param_count != 0) {
            report_error(9, node->line, "Function \"%s\" is not applicable for arguments", func->name);
        }

        return copy_type(func->return_type);
    }

    if (is_node(first, "Exp") && is_node(second, "LB")) {
        Type *array = analyze_exp(first);
        Type *index = analyze_exp(child(node, 2));
        Type *result = NULL;

        if (array != NULL && array->kind != TYPE_ARRAY) {
            report_error(10, node->line, "Not an array");
        } else if (index != NULL && !is_int_type(index)) {
            report_error(12, child(node, 2)->line, "Array index is not an integer");
        } else if (array != NULL) {
            result = copy_type(array->u.array.elem);
        }

        free_type(array);
        free_type(index);
        return result;
    }

    if (is_node(first, "Exp") && is_node(second, "DOT")) {
        Type *base = analyze_exp(first);
        Type *result = NULL;
        TreeNode id = child(node, 2);

        if (base != NULL && base->kind != TYPE_STRUCT) {
            report_error(13, node->line, "Illegal use of \".\"");
        } else if (base != NULL) {
            Field *field = find_field(base->u.structure.fields, id->value.text);
            if (field == NULL) {
                report_error(14, id->line, "Non-existent field \"%s\"", id->value.text);
            } else {
                result = copy_type(field->type);
            }
        }

        free_type(base);
        return result;
    }

    if (is_node(first, "Exp")) {
        Type *left = analyze_exp(first);
        Type *right = analyze_exp(child(node, 2));
        Type *result = NULL;

        if (is_node(second, "ASSIGNOP")) {
            if (!is_lvalue(first)) {
                report_error(6, node->line, "The left-hand side of an assignment must be a variable");
            } else if (!same_type(left, right)) {
                report_error(5, node->line, "Type mismatched for assignment");
            } else {
                result = copy_type(left);
            }
        } else if (is_node(second, "AND") || is_node(second, "OR")) {
            if ((left != NULL && !is_int_type(left)) || (right != NULL && !is_int_type(right))) {
                report_error(7, node->line, "Type mismatched for operands");
            } else if (left != NULL && right != NULL) {
                result = new_basic_type(BASIC_INT);
            }
        } else if (is_node(second, "RELOP")) {
            if ((left != NULL && !is_numeric_type(left)) ||
                (right != NULL && !is_numeric_type(right)) ||
                !same_type(left, right)) {
                report_error(7, node->line, "Type mismatched for operands");
            } else if (left != NULL && right != NULL) {
                result = new_basic_type(BASIC_INT);
            }
        } else {
            if ((left != NULL && !is_numeric_type(left)) ||
                (right != NULL && !is_numeric_type(right)) ||
                !same_type(left, right)) {
                report_error(7, node->line, "Type mismatched for operands");
            } else if (left != NULL && right != NULL) {
                result = copy_type(left);
            }
        }

        free_type(left);
        free_type(right);
        return result;
    }

    return NULL;
}

static int analyze_args(TreeNode node, FuncSymbol *func) {
    Field *formal = func->params;
    TreeNode cur = node;
    int mismatch = 0;

    while (!is_empty(cur)) {
        Type *actual = analyze_exp(child(cur, 0));
        if (formal == NULL) {
            mismatch = 1;
        } else if (!same_type(actual, formal->type)) {
            mismatch = 1;
        } else {
            formal = formal->next;
        }
        free_type(actual);

        if (child(cur, 1) == NULL) {
            break;
        }
        cur = child(cur, 2);
    }

    if (formal != NULL) {
        mismatch = 1;
    }

    if (mismatch) {
        report_error(9, node->line, "Function \"%s\" is not applicable for arguments", func->name);
    }
    return !mismatch;
}


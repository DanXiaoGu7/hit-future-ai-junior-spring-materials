#ifndef _SEMANTICS_H_
#define _SEMANTICS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "treenode.h"

#define SIZE 0x3fff
#define T_INT 0
#define T_FLOAT 1

#define DEF_STRUCT 0
#define DEF_VAR 1
#define DEF_FUNC 2

#define DEF_NOT_IN_STRUCT 0
#define DEF_IN_STRUCT 1

typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct Vari_ *Vari;
typedef struct Function_ *Function;
struct Type_
{
    enum
    {
        BASIC,
        ARRAY,
        STRUCT
    } type;
    int basic; // 0表示int 1表示float
    struct
    {
        Type elem;
        int size;
    } array;             //元素类型+数组大小
    FieldList structure; //结构体类型
};
struct FieldList_
{
    char *name;
    Type type;
    FieldList next;
};
struct Vari_
{
    int is_def_struct;
    FieldList field;
    Vari next;
    int line;
};
struct Function_
{
    char *name;
    FieldList field;
    Type return_type;
    int defined;
    int line;
    Function next;
};

// hash function
unsigned int hash_pjw(char *name);

// check if a name is in the table
Vari find_vari_table(char *name);

// add a field to the table
void insert_vari_table(FieldList field_list, int line, int in_struct);

// compare two types
int typecmp(Type a, Type b);
int fieldcmp(FieldList a, FieldList b, int compare_name);

Function find_function_table(char *name);
// add a function to the function table
void insert_function_table(Function func);

// compare two functions
int funccmp(Function a, Function b);

// -----------------semantic analysis------------------------
// ---------------------program---------------------------
void semantic_analysis(Treenode now);

// def of global variables, functions, structs
void ExtDef(Treenode now);
// def of global variables
void ExtDecList(Treenode now, Type type);
// description of type, including TYPE and struct
Type Specifier(Treenode now);
// description of struct
Type StructSpecifier(Treenode now);
// name of struct
char *OptTag(Treenode now);

// def list of variables, functions, structs (local)
FieldList DefList(Treenode now, int judge);

// def of variables, functions, structs (local)
FieldList Def(Treenode now, int judge);

// name list of variables, functions, structs (local)
FieldList DecList(Treenode now, Type type, int judge);

// name of variables, functions, structs (local)
FieldList Dec(Treenode now, Type type, int judge);

// def of variables
FieldList VarDec(Treenode now, Type type, int judge);

void FunDec(Treenode now, Type type, int judge);
// paramlist of functions
FieldList VarList(Treenode now, int judge);
// param def of functions
FieldList ParamDec(Treenode now, int judge);

// statements of functions
void CompSt(Treenode now, Type type);
// list of statements
void StmtList(Treenode now, Type type);
// statement
void Stmt(Treenode now, Type type);
Type Exp(Treenode now);
FieldList Args(Treenode now);

#endif
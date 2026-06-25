#ifndef INTERIM_H
#define INTERIM_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "semantics.h"

#define INTERCODE_1 1
#define INTERCODE_2 2
#define INTERCODE_3 3
#define INTERCODE_IF 4
#define INTERCODE_DEC 12
#define OPERAND_NULL -1

typedef struct Operand_ *Operand;
typedef struct InterCode_ *InterCode;

enum OperandKind
{
    VARIABLE,
    CONSTANT,
    ADDRESS,
    FUNCTION,
    LABEL,
    RELOP,
    UNKNOWN
};

// 操作数结构体
struct Operand_ 
{
    enum OperandKind kind;  // 操作数的类型（变量、常量、地址等）
    int u_int;              // 存储整型数据，例如常量值或变量的索引
    char *u_char;           // 存储字符串数据，如变量名或函数名
    Type type;              // 操作数的数据类型信息，来自语义分析的结果
};


enum InterCodeKind
{
    ILABEL,
    IFUNCTION,
    ASSIGN,
    ADD,
    SUB,
    MUL,
    DIV,
    ADDRASS1,
    ADDRASS2,
    ADDRASS3,
    GOTO,
    IF,
    RETURN,
    DEC,
    ARG,
    CALL,
    PARAM,
    READ,
    WRITE
};

// 中间代码结构体
struct InterCode_
{
    enum InterCodeKind kind;  // 中间代码的种类（赋值、算术运算、条件跳转等）
    union
    {
        struct // 用于LABEL, FUNCTION, GOTO, RETURN, ARG, PARAM, READ, WRITE等单一操作数指令
        {
            Operand op;
        } operand_1;
        struct // 用于ASSIGN, CALL, ADDRASS1, ADDRASS2, ADDRASS3等双操作数指令
        {
            Operand op1, op2;
        } operand_2;
        struct // 用于ADD, SUB, MUL, DIV等三操作数指令
        {
            Operand result, op1, op2;
        } operand_3;
        struct // 用于IF语句，包括条件操作数和跳转标签
        {
            Operand x, relop, y, z;
        } operand_if;
        struct // 用于DEC指令，用于声明数组或结构体大小
        {
            Operand op;
            int size;
        } operand_dec;
    };
    InterCode prev;  // 指向前一个中间代码的指针
    InterCode next;  // 指向下一个中间代码的指针
};


Operand new_temp();
Operand new_label();
void translate_print(FILE *f);

void translate_Program(Treenode now, FILE *F);
void translate_ExtDefList(Treenode now);
void translate_ExtDef(Treenode now);
void translate_FunDec(Treenode now);
void translate_CompSt(Treenode now);
void translate_DefList(Treenode now);
void translate_StmtList(Treenode now);
void translate_Def(Treenode now);
void translate_Stmt(Treenode now);
void translate_DecList(Treenode now);
void translate_Exp(Treenode now, Operand place);
void translate_CompSt(Treenode now);
void translate_Cond(Treenode now, Operand lt, Operand lf);
void translate_Dec(Treenode now);
void translate_VarDec(Treenode now, Operand place);
void translate_Args(Treenode now, InterCode here);

void insert_intercode(InterCode this);
int get_offset(Type return_type);
int get_size(Type type);

#endif
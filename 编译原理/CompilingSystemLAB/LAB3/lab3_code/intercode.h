#ifndef INTERCODE_H
#define INTERCODE_H

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

typedef struct Operand_d Operand_;
typedef Operand_* Operand;
typedef struct InterCode_d InterCode_;
typedef InterCode_* InterCode;

struct Operand_d {
    enum {
        VARIABLE_OP, TEMP_VAR_OP, CONSTANT_OP, LABEL_OP, 
        FUNCTION_OP, GET_ADDR_OP, GET_VAL_OP
    } kind;
    union {
        int no; 
        int value;  
        char name[32];  
        Operand opr; 
    };
    Type type;  
    Operand next;   
};

struct InterCode_d {
    enum {
        LABEL_IR, FUNC_IR, ASSIGN_IR, PLUS_IR, SUB_IR, MUL_IR, 
        DIV_IR, TO_MEM_IR, GOTO_IR,
        IF_GOTO_IR, RETURN_IR, DEC_IR, ARG_IR, CALL_IR, PARAM_IR,
        READ_IR, WRITE_IR, NULL_IR
    } kind;

    Operand ops[3];

    union {
        char relop[32];
        int size;
    };

    InterCode pre;
    InterCode next;
};

void initInterCodes();
void insertInterCode(InterCode code, InterCode interCodes);
void printInterCodes(char* name);
void printOperand(Operand op, FILE* fp);

InterCode translateExp(Node* root, Operand place);
InterCode translateArgs(Node* root, Operand argList);
InterCode translateStmt(Node* root);
InterCode translateCond(Node* root, Operand labelTrue, Operand labelFalse);
void translateProgram(Node* root);
InterCode translateExtDefList(Node* root);
InterCode translateExtDef(Node* root);
InterCode translateCompSt(Node* root, char* funcName);
InterCode translateStmtList(Node* root);
InterCode translateDefList(Node* root, IdType class);
InterCode translateDef(Node* root, IdType class);
FieldList translateDecList(Node* root, Type type, IdType class, InterCode code);
FieldList translateDec(Node* root, Type type, IdType class, InterCode code);

#endif
%{
#include <stdio.h>
#include <string.h>

#include "treenode.h"

int yylex(void);
int yyerror(const char *msg);

extern int yylineno;
extern char *yytext;
extern int has_fault;
extern int last_error_line;

TreeNode root;

static void mark_syntax_error(int line, const char *message) {
    if (last_error_line == line) {
        return;
    }
    last_error_line = line;
    has_fault = 1;
    printf("Error type B at Line %d: %s.\n", line, message);
}
%}

%code requires {
#include "treenode.h"
}

%locations

%union {
    struct ParseTree *node;
}

%token <node> INT FLOAT ID
%token <node> SEMI COMMA
%token <node> ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR DOT NOT
%token <node> LP RP LB RB LC RC
%token <node> TYPE STRUCT RETURN
%token <node> IF ELSE WHILE

%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def Dec DecList
%type <node> Exp Args

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

Program
    : ExtDefList                         { $$ = new_node(@1.first_line, "Program", 1, $1); root = $$; }
    ;

ExtDefList
    : ExtDef ExtDefList                  { $$ = new_node(@1.first_line, "ExtDefList", 2, $1, $2); }
    | %empty                             { $$ = new_empty_node(); }
    ;

ExtDef
    : Specifier ExtDecList SEMI          { $$ = new_node(@1.first_line, "ExtDef", 3, $1, $2, $3); }
    | Specifier SEMI                     { $$ = new_node(@1.first_line, "ExtDef", 2, $1, $2); }
    | Specifier FunDec CompSt            { $$ = new_node(@1.first_line, "ExtDef", 3, $1, $2, $3); }
    | error SEMI                         { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Specifier error                    { mark_syntax_error(@1.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Specifier error SEMI               { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

ExtDecList
    : VarDec                             { $$ = new_node(@1.first_line, "ExtDecList", 1, $1); }
    | VarDec COMMA ExtDecList            { $$ = new_node(@1.first_line, "ExtDecList", 3, $1, $2, $3); }
    | VarDec error COMMA ExtDecList      { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

Specifier
    : TYPE                               { $$ = new_node(@1.first_line, "Specifier", 1, $1); }
    | StructSpecifier                    { $$ = new_node(@1.first_line, "Specifier", 1, $1); }
    ;

StructSpecifier
    : STRUCT OptTag LC DefList RC        { $$ = new_node(@1.first_line, "StructSpecifier", 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag                         { $$ = new_node(@1.first_line, "StructSpecifier", 2, $1, $2); }
    | STRUCT error LC DefList RC         { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | STRUCT OptTag LC error RC          { mark_syntax_error(@4.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | STRUCT OptTag LC error             { mark_syntax_error(@4.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | STRUCT error                       { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

OptTag
    : ID                                 { $$ = new_node(@1.first_line, "OptTag", 1, $1); }
    | %empty                             { $$ = new_empty_node(); }
    ;

Tag
    : ID                                 { $$ = new_node(@1.first_line, "Tag", 1, $1); }
    ;

VarDec
    : ID                                 { $$ = new_node(@1.first_line, "VarDec", 1, $1); }
    | VarDec LB INT RB                   { $$ = new_node(@1.first_line, "VarDec", 4, $1, $2, $3, $4); }
    | VarDec LB error RB                 { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | VarDec LB error                    { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

FunDec
    : ID LP VarList RP                   { $$ = new_node(@1.first_line, "FunDec", 4, $1, $2, $3, $4); }
    | ID LP RP                           { $$ = new_node(@1.first_line, "FunDec", 3, $1, $2, $3); }
    | ID LP error RP                     { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

VarList
    : ParamDec COMMA VarList             { $$ = new_node(@1.first_line, "VarList", 3, $1, $2, $3); }
    | ParamDec                           { $$ = new_node(@1.first_line, "VarList", 1, $1); }
    ;

ParamDec
    : Specifier VarDec                   { $$ = new_node(@1.first_line, "ParamDec", 2, $1, $2); }
    ;

CompSt
    : LC DefList StmtList RC             { $$ = new_node(@1.first_line, "CompSt", 4, $1, $2, $3, $4); }
    ;

StmtList
    : Stmt StmtList                      { $$ = new_node(@1.first_line, "StmtList", 2, $1, $2); }
    | %empty                             { $$ = new_empty_node(); }
    ;

Stmt
    : Exp SEMI                           { $$ = new_node(@1.first_line, "Stmt", 2, $1, $2); }
    | CompSt                             { $$ = new_node(@1.first_line, "Stmt", 1, $1); }
    | RETURN Exp SEMI                    { $$ = new_node(@1.first_line, "Stmt", 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
                                         { $$ = new_node(@1.first_line, "Stmt", 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt        { $$ = new_node(@1.first_line, "Stmt", 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt               { $$ = new_node(@1.first_line, "Stmt", 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Exp ELSE Stmt         { mark_syntax_error(@6.first_line, "Missing \";\""); $$ = new_empty_node(); }
    | IF LP Exp RP error ELSE Stmt       { mark_syntax_error(@5.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE
                                         { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | IF LP error RP Stmt ELSE Stmt      { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | error LP Exp RP Stmt               { mark_syntax_error(@1.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

DefList
    : Def DefList                        { $$ = new_node(@1.first_line, "DefList", 2, $1, $2); }
    | %empty                             { $$ = new_empty_node(); }
    ;

Def
    : Specifier DecList SEMI             { $$ = new_node(@1.first_line, "Def", 3, $1, $2, $3); }
    ;

DecList
    : Dec                                { $$ = new_node(@1.first_line, "DecList", 1, $1); }
    | Dec COMMA DecList                  { $$ = new_node(@1.first_line, "DecList", 3, $1, $2, $3); }
    | Dec error DecList                  { mark_syntax_error(@2.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

Dec
    : VarDec                             { $$ = new_node(@1.first_line, "Dec", 1, $1); }
    | VarDec ASSIGNOP Exp                { $$ = new_node(@1.first_line, "Dec", 3, $1, $2, $3); }
    ;

Exp
    : Exp ASSIGNOP Exp                   { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp AND Exp                        { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp OR Exp                         { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp RELOP Exp                      { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp PLUS Exp                       { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp MINUS Exp                      { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp STAR Exp                       { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp DIV Exp                        { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | LP Exp RP                          { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | MINUS Exp                          { $$ = new_node(@1.first_line, "Exp", 2, $1, $2); }
    | NOT Exp                            { $$ = new_node(@1.first_line, "Exp", 2, $1, $2); }
    | ID LP Args RP                      { $$ = new_node(@1.first_line, "Exp", 4, $1, $2, $3, $4); }
    | ID LP RP                           { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | Exp LB Exp RB                      { $$ = new_node(@1.first_line, "Exp", 4, $1, $2, $3, $4); }
    | Exp LB Exp COMMA Exp RB            { mark_syntax_error(@4.first_line, "Missing \"]\""); $$ = new_empty_node(); }
    | Exp DOT ID                         { $$ = new_node(@1.first_line, "Exp", 3, $1, $2, $3); }
    | ID                                 { $$ = new_node(@1.first_line, "Exp", 1, $1); }
    | INT                                { $$ = new_node(@1.first_line, "Exp", 1, $1); }
    | FLOAT                              { $$ = new_node(@1.first_line, "Exp", 1, $1); }
    | Exp LB error RB                    { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp ASSIGNOP error                 { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp AND error                      { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp OR error                       { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp RELOP error                    { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp PLUS error                     { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp MINUS error                    { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp STAR error                     { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | Exp DIV error                      { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    | ID LP error RP                     { mark_syntax_error(@3.first_line, "Syntax error"); $$ = new_empty_node(); yyerrok; }
    ;

Args
    : Exp COMMA Args                     { $$ = new_node(@1.first_line, "Args", 3, $1, $2, $3); }
    | Exp                                { $$ = new_node(@1.first_line, "Args", 1, $1); }
    ;

%%

int yyerror(const char *msg) {
    int line = yylloc.first_line > 0 ? yylloc.first_line : yylineno;
    mark_syntax_error(line, strcmp(msg, "syntax error") == 0 ? "Syntax error" : msg);
    return 0;
}

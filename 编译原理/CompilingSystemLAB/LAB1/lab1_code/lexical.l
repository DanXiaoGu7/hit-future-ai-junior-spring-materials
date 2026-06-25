%option noyywrap nodefault yylineno noinput nounput

%{
#include <stdio.h>
#include <string.h>

#include "syntax.tab.h"

int yycolumn = 1;
int last_error_line = 0;

extern int has_fault;

static void report_lexical_error(const char *message) {
    if (last_error_line == yylineno) {
        return;
    }
    last_error_line = yylineno;
    has_fault = 1;
    printf("Error type A at Line %d: %s.\n", yylineno, message);
}

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

DIGIT           [0-9]
LETTER          [a-zA-Z_]
ID              {LETTER}[a-zA-Z0-9_]*
DEC             0|([1-9][0-9]*)
INT             {DEC}
FLOAT           ({DIGIT}+\.{DIGIT}*)|({DIGIT}*\.{DIGIT}+)
WHITESPACE      [ \t]+
NEWLINE         \r\n|\n|\r

%%

{WHITESPACE}    {}
{NEWLINE}       { yycolumn = 1; }

";"             { yylval.node = new_node(yylineno, "SEMI", 0); return SEMI; }
","             { yylval.node = new_node(yylineno, "COMMA", 0); return COMMA; }
"="             { yylval.node = new_node(yylineno, "ASSIGNOP", 0); return ASSIGNOP; }
">="|"<="|"=="|"!="|">"|"<" { yylval.node = new_node(yylineno, "RELOP", 0); return RELOP; }
"+"             { yylval.node = new_node(yylineno, "PLUS", 0); return PLUS; }
"-"             { yylval.node = new_node(yylineno, "MINUS", 0); return MINUS; }
"*"             { yylval.node = new_node(yylineno, "STAR", 0); return STAR; }
"/"             { yylval.node = new_node(yylineno, "DIV", 0); return DIV; }
"&&"            { yylval.node = new_node(yylineno, "AND", 0); return AND; }
"||"            { yylval.node = new_node(yylineno, "OR", 0); return OR; }
"."             { yylval.node = new_node(yylineno, "DOT", 0); return DOT; }
"!"             { yylval.node = new_node(yylineno, "NOT", 0); return NOT; }
"("             { yylval.node = new_node(yylineno, "LP", 0); return LP; }
")"             { yylval.node = new_node(yylineno, "RP", 0); return RP; }
"["             { yylval.node = new_node(yylineno, "LB", 0); return LB; }
"]"             { yylval.node = new_node(yylineno, "RB", 0); return RB; }
"{"             { yylval.node = new_node(yylineno, "LC", 0); return LC; }
"}"             { yylval.node = new_node(yylineno, "RC", 0); return RC; }
"struct"        { yylval.node = new_node(yylineno, "STRUCT", 0); return STRUCT; }
"return"        { yylval.node = new_node(yylineno, "RETURN", 0); return RETURN; }
"if"            { yylval.node = new_node(yylineno, "IF", 0); return IF; }
"else"          { yylval.node = new_node(yylineno, "ELSE", 0); return ELSE; }
"while"         { yylval.node = new_node(yylineno, "WHILE", 0); return WHILE; }
"int"|"float"   { yylval.node = new_node(yylineno, "TYPE", 0); return TYPE; }

{FLOAT}         { yylval.node = new_node(yylineno, "FLOAT", 0); return FLOAT; }
{INT}           { yylval.node = new_node(yylineno, "INT", 0); return INT; }
{ID}            { yylval.node = new_node(yylineno, "ID", 0); return ID; }

.               {
                    char message[256];
                    snprintf(message, sizeof(message), "Mysterious character \"%s\"", yytext);
                    report_lexical_error(message);
                }

%%

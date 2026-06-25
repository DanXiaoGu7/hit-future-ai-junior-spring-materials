%{
	#include"syntax.tab.h"
	#include"treenode.h"
	int yycolumn=1;
	extern int hasFault;
	#define YY_USER_ACTION \
		yylloc.first_line=yylloc.last_line=yylineno; \
		yylloc.first_column=yycolumn; \
		yylloc.last_column=yycolumn+yyleng-1; \
		yycolumn+=yyleng;
%}
%option yylineno
INT [1-9][0-9]*|0
FLOAT {INT}\.[0-9]+
ID [a-zA-Z_][a-zA-Z_0-9]*
ENTER [\n\r]+
SPACE [ \t]+
WHILE while
ELSE else
IF if
RETURN return
STRUCT struct
LC "{"
RC "}"
LB "["
RB "]"
LP "("
RP ")"
TYPE int|float
NOT "!"
DOT "."
OR "||"
AND "&&"
DIV "/"
STAR "*"
MINUS "-"
PLUS "+"
RELOP ">"|"<"|">="|"<="|"=="|"!="
ASSIGNOP "="
COMMA ","
SEMI ";"
%%
{SPACE} {}
{ENTER} {yycolumn = 1;}
{SEMI} {yylval.node = newnode(yylineno, "SEMI", 0); return SEMI;} 
{COMMA} {yylval.node = newnode(yylineno, "COMMA", 0); return COMMA;} 
{ASSIGNOP} {yylval.node = newnode(yylineno, "ASSIGNOP", 0); return ASSIGNOP;} 
{RELOP} {yylval.node = newnode(yylineno, "RELOP", 0); return RELOP;} 
{PLUS} { yylval.node = newnode(yylineno, "PLUS", 0); return PLUS;} 
{MINUS} {yylval.node = newnode(yylineno, "MINUS", 0); return MINUS;} 
{STAR} {yylval.node = newnode(yylineno, "STAR", 0); return STAR;} 
{DIV} {yylval.node = newnode(yylineno, "DIV", 0); return DIV;} 
{AND} {yylval.node = newnode(yylineno, "AND", 0); return AND;} 
{OR} {yylval.node = newnode(yylineno, "OR", 0); return OR;}  
{DOT} {yylval.node = newnode(yylineno, "DOT", 0); return DOT;} 
{NOT} {yylval.node = newnode(yylineno, "NOT", 0); return NOT;} 
{TYPE} {yylval.node = newnode(yylineno, "TYPE", 0); return TYPE;} 
{LP} {yylval.node = newnode(yylineno, "LP", 0); return LP;}  
{RP} {yylval.node = newnode(yylineno, "RP", 0); return RP;}  
{LB} {yylval.node = newnode(yylineno, "LB", 0); return LB;}  
{RB} {yylval.node = newnode(yylineno, "RB", 0); return RB;}  
{LC} {yylval.node = newnode(yylineno, "LC", 0); return LC;}  
{RC} {yylval.node = newnode(yylineno, "RC", 0); return RC;}  
{STRUCT} {yylval.node = newnode(yylineno, "STRUCT", 0); return STRUCT;} 
{RETURN} {yylval.node = newnode(yylineno, "RETURN", 0); return RETURN;} 
{IF} { yylval.node = newnode(yylineno, "IF", 0); return IF;} 
{ELSE} {yylval.node = newnode(yylineno, "ELSE", 0); return ELSE;} 
{WHILE} {yylval.node = newnode(yylineno, "WHILE", 0); return WHILE;} 
{INT} {yylval.node = newnode(yylineno, "INT", 0); return INT;} ; 
{FLOAT} {yylval.node = newnode(yylineno, "FLOAT", 0); return FLOAT;} 
{ID} {yylval.node = newnode(yylineno, "ID", 0); return ID;}  
{INT}+{ID}  {hasFault = 1; printf("Error type A at Line %d: Illegal ID \'%s\'\n", yylineno, yytext); }
\.{INT} {hasFault = 1; printf("Error type A at Line %d: Illegal float number \'%s\'\n", yylineno, yytext); }
. {hasFault = 1; printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }
%%

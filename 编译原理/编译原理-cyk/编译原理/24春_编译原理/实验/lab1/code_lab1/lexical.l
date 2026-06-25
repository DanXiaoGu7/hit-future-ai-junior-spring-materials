%{
	#include"treenode.h"
	int yycolumn=1;
	extern int hasFault;
	int line=1;
%}
%option yylineno//跟踪行号
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
{ENTER} {line++;}
{SEMI} {printf("line:%d:;\n",line);} 
{COMMA} {printf("line:%d:,\n",line);} 
{ASSIGNOP} {printf("line:%d:=\n",line);} 
{RELOP} {printf("line:%d:%s\n",line,yytext);} 
{PLUS} {printf("line:%d:+\n",line);} 
{MINUS} {printf("line:%d:-\n",line);} 
{STAR} {printf("line:%d:*\n",line);} 
{DIV} {printf("line:%d:/\n",line);} 
{AND} {printf("line:%d:&&\n",line);} 
{OR} {printf("line:%d:||\n",line);}  
{DOT} {printf("line:%d:.\n",line);} 
{NOT} {printf("line:%d:!\n",line);} 
{TYPE} {printf("line:%d:%s\n",line,yytext);} 
{LP} {printf("line:%d:(\n",line);}  
{RP} {printf("line:%d:)\n",line);}  
{LB} {printf("line:%d:[\n",line);}  
{RB} {printf("line:%d:]\n",line);}  
{LC} {printf("line:%d:{\n",line);}  
{RC} {printf("line:%d:}\n",line);}  
{STRUCT} {printf("line:%d:struct\n",line);} 
{RETURN} {printf("line:%d:return\n",line);} 
{IF} { printf("line:%d:if\n",line);} 
{ELSE} {printf("line:%d:else\n",line);} 
{WHILE} {printf("line:%d:while\n",line);} 
{INT} {printf("line:%d:%s\n",line,yytext);} ; 
{FLOAT} {printf("line:%d:%s\n",line,yytext);} 
{ID} {printf("line:%d:%s\n",line,yytext);}  
{INT}+{ID}  {hasFault = 1; printf("Error type A at Line %d: Illegal ID \'%s\'\n", yylineno, yytext); }
\.{INT} {hasFault = 1; printf("Error type A at Line %d: Illegal float number \'%s\'\n", yylineno, yytext); }
. {hasFault = 1; printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }
%%

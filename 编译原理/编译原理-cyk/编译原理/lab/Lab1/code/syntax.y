%{
    #include "lex.yy.c" // 包含词法分析器生成的源文件
    #include "treenode.h" // 包含树节点头文件
    int yyerror(char* msg); // 声明语法错误处理函数
    extern int hasFault; // 声明是否存在错误标志
    Treenode root; // 声明根节点
%}
/* declared types */
%union {
    struct Parsetree* node; // 定义联合类型，用于存储语法树节点指针
}
/* declared tokens */
%token <node> INT FLOAT ID // 定义终结符
%token <node> SEMI COMMA
%token <node> ASSIGNOP RELOP 
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR DOT NOT
%token <node> LP RP LB RB LC RC
%token <node> TYPE STRUCT RETURN
%token <node> IF ELSE WHILE
//%token SPACE ENTER

// non-terminals
%type <node> Program ExtDefList ExtDef ExtDecList   //  High-level Definitions
%type <node> Specifier StructSpecifier OptTag Tag   //  Specifiers
%type <node> VarDec FunDec VarList ParamDec         //  Declarators
%type <node> CompSt StmtList Stmt                   //  Statements
%type <node> DefList Def Dec DecList                //  Local Definitions
%type <node> Exp Args                               //  Expressions

/* Priority and Associativity */
%right ASSIGNOP // 右结合的赋值操作符
%left OR // 左结合的逻辑或操作符
%left AND // 左结合的逻辑与操作符
%left RELOP // 左结合的关系操作符
%left PLUS MINUS // 左结合的加减操作符
%left STAR DIV // 左结合的乘除操作符
%right NOT // 右结合的逻辑非操作符
%left LP RP LB RB DOT // 左结合的括号和点操作符
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE 

%%
/* High-level Definitions */
Program : ExtDefList {$$ = newnode (@$.first_line, "Program", 1, $1); root = $$;} /* 定义根节点 */
    ;
ExtDefList : ExtDef ExtDefList {$$ = newnode(@$.first_line, "ExtDefList", 2, $1, $2);}
    | /* empty */ {$$ = EmptyNode();}
    ;
ExtDef : Specifier ExtDecList SEMI {$$ = newnode(@$.first_line, "ExtDef", 3, $1, $2, $3);}
    | Specifier SEMI {$$ = newnode(@$.first_line, "ExtDef", 2, $1, $2);}
    | Specifier FunDec CompSt {$$ = newnode(@$.first_line, "ExtDef", 3, $1, $2, $3);}
    | error SEMI {hasFault = 1;} // 错误处理
    | Specifier error {hasFault = 1;}
    | Specifier error SEMI {hasFault = 1;}
    ;
ExtDecList : VarDec {$$ = newnode(@$.first_line, "ExtDecList", 1, $1);}
    | VarDec COMMA ExtDecList {$$ = newnode(@$.first_line, "ExtDecList", 3, $1, $2, $3);}
    | VarDec error COMMA ExtDecList {hasFault = 1;}
    ;

/* Specifiers */
Specifier : TYPE {$$ = newnode(@$.first_line, "Specifier", 1, $1);}
    | StructSpecifier {$$ = newnode(@$.first_line, "Specifier", 1, $1);}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC {$$ = newnode(@$.first_line, "StructSpecifier", 5, $1, $2, $3, $4, $5);}
    | STRUCT Tag {$$ = newnode(@$.first_line, "StructSpecifier", 2, $1, $2);}
    |		    STRUCT error LC DefList RC		    { hasFault = 1;}
    |		    STRUCT OptTag LC error RC		    { hasFault = 1;}
    |		    STRUCT OptTag LC error 		    { hasFault = 1;}
    |		    STRUCT error			    { hasFault = 1; }
    ;
OptTag : ID {$$ = newnode(@$.first_line, "OptTag", 1, $1);}
    | /* empty */ {$$ = EmptyNode();}
    ;
Tag : ID {$$ = newnode(@$.first_line, "Tag", 1, $1);}
    ;

/* Declarators */
VarDec : ID {$$ = newnode(@$.first_line, "VarDec", 1, $1);}
    | VarDec LB INT RB {$$ = newnode(@$.first_line, "VarDec", 4, $1, $2, $3, $4);}
    | VarDec LB error RB { hasFault = 1; }
    | VarDec LB error { hasFault = 1; } 
    ;
FunDec : ID LP VarList RP {$$ = newnode(@$.first_line, "FunDec", 4, $1, $2, $3, $4);}
    | ID LP RP {$$ = newnode(@$.first_line, "FunDec", 3, $1, $2, $3);}
    //| error RP {hasFault = 1;} // 错误处理
    | ID LP error RP {hasFault = 1;}
    ;
VarList : ParamDec COMMA VarList {$$ = newnode(@$.first_line, "VarList", 3, $1, $2, $3);}
    | ParamDec {$$ = newnode(@$.first_line, "VarList", 1, $1);}
    ;
ParamDec : Specifier VarDec {$$ = newnode(@$.first_line, "ParamDec", 2, $1, $2);}
    ;

/* Statements */
CompSt : LC DefList StmtList RC {$$ = newnode(@$.first_line, "CompSt", 4, $1, $2, $3, $4);}
    //| error RC {hasFault = 1;} // 错误处理
    ;
StmtList : Stmt StmtList {$$ = newnode(@$.first_line, "StmtList", 2, $1, $2);}
    | /* empty */ {$$ = EmptyNode();}
    ;
Stmt : Exp SEMI {$$ = newnode(@$.first_line, "Stmt", 2, $1, $2);}
    | CompSt {$$ = newnode(@$.first_line, "Stmt", 1, $1);}
    | RETURN Exp SEMI {$$ = newnode(@$.first_line, "Stmt", 3, $1, $2, $3);}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{$$ = newnode(@$.first_line, "Stmt", 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt {$$ = newnode(@$.first_line, "Stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
    | WHILE LP Exp RP Stmt {$$ = newnode(@$.first_line, "Stmt", 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP error ELSE Stmt {hasFault = 1;} // 错误处理
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE {hasFault = 1;}
    | IF LP error RP Stmt ELSE Stmt {hasFault = 1;}
    | error LP Exp RP Stmt {hasFault = 1;}
    ;

/* Local Definitions */
DefList : Def DefList {$$ = newnode(@$.first_line, "DefList", 2, $1, $2);}
    | /* empty */ {$$ = EmptyNode();}
    ;
Def : Specifier DecList SEMI {$$ = newnode(@$.first_line, "Def", 3, $1, $2, $3);}
    //| error SEMI {hasFault = 1;} // 错误处理
    ;
DecList : Dec {$$ = newnode(@$.first_line, "DecList", 1, $1);}
    | Dec COMMA DecList {$$ = newnode(@$.first_line, "DecList", 3, $1, $2, $3);}
    | Dec error DecList {hasFault = 1;}
    ;
Dec : VarDec {$$ = newnode(@$.first_line, "Dec", 1, $1);}
    | VarDec ASSIGNOP Exp {$$ = newnode(@$.first_line, "Dec", 3, $1, $2, $3);}
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp AND Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp OR Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp RELOP Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp PLUS Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp MINUS Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp STAR Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp DIV Exp {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | LP Exp RP {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | MINUS Exp {$$ = newnode(@$.first_line, "Exp", 2, $1, $2);}
    | NOT Exp {$$ = newnode(@$.first_line, "Exp", 2, $1, $2);}
    | ID LP Args RP {$$ = newnode(@$.first_line, "Exp", 4, $1, $2, $3, $4);}
    | ID LP RP {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp LB Exp RB {$$ = newnode(@$.first_line, "Exp", 4, $1, $2, $3, $4);}
    | Exp DOT ID {$$ = newnode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | ID {$$ = newnode(@$.first_line, "Exp", 1, $1);}
    | INT {$$ = newnode(@$.first_line, "Exp", 1, $1);}
    | FLOAT {$$ = newnode(@$.first_line, "Exp", 1, $1);}
    | Exp LB error RB {hasFault = 1;}
    | Exp ASSIGNOP error {hasFault = 1;}
    | Exp AND error {hasFault = 1;}
    | Exp OR error {hasFault = 1;}     	  	    
    | Exp RELOP error {hasFault = 1;}      	   	    
    | Exp PLUS error  {hasFault = 1;}      		    
    | Exp MINUS error {hasFault = 1;}      		    
    | Exp STAR error  {hasFault = 1;}      		    
    | Exp DIV error {hasFault = 1;}        	   	
    | ID LP error RP {hasFault = 1;}
    //| error RP {hasFault = 1;} // 错误处理
    //| error RB {hasFault = 1;} // 错误处理
    ;
Args : Exp COMMA Args {$$ = newnode(@$.first_line, "Args", 3, $1, $2, $3);}
    | Exp {$$ = newnode(@$.first_line, "Args", 1, $1);}
    ;
%%
int yyerror(char* msg){ // 定义语法错误处理函数
    fprintf(stderr, "Error type B at Line %d: %s, near\'%s\'\n", yylineno, msg, yytext); // 输出错误信息
}

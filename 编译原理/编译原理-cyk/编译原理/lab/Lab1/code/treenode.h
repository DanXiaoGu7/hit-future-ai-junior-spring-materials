/*二叉树表示多叉树数据结构*/
#ifndef _TREENODE_H_
#define _TREENODE_H_


#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

extern int yylineno;
extern char *yytext;
extern int hasFault;
int yyerror(char *msg);

struct Parsetree
{
	int line;
	char *Token;
	int isleaf;
	union
	{
		char* Id_Type;
		int intval;
		float floatval;
	};
	struct Parsetree *firstchild,*nxxtbro;
};
typedef struct Parsetree* Treenode;

static Treenode EmptyNode()
{
	Treenode ret=(Treenode)malloc(sizeof(struct Parsetree));
	ret->Token="EMPTY";
	ret->firstchild=NULL;
	ret->nxxtbro=NULL;
	return ret;
}
static Treenode newnode(int line,char *TOKEN,int amount,...)
{
	if(hasFault) return NULL;
	Treenode rt=(Treenode)malloc(sizeof(struct Parsetree));
	Treenode ch=(Treenode)malloc(sizeof(struct Parsetree));
	if (rt==NULL)
	{
		yyerror("Create Parsetree Error!");
		exit(0);
	}
	rt->line=line;
	rt->Token=TOKEN;
	rt->isleaf=0;
	if (amount==0)
	{
		rt->isleaf=1;
		rt->firstchild=NULL;
		if((!strcmp(TOKEN,"ID"))||(!strcmp(TOKEN,"TYPE")))
		{
			char *str;
			str=(char*)malloc(sizeof(char)*40);
			strcpy(str,yytext);
			rt->Id_Type=str;
		}
		else if(!strcmp(TOKEN,"INT"))
			rt->intval=strtol(yytext,NULL,0);
		else if(!strcmp(TOKEN,"FLOAT"))
			rt->floatval=atof(yytext);
	}
	else
	{
		va_list list;
		va_start(list,amount);
		ch=va_arg(list,Treenode);
		rt->firstchild=ch;
		if (amount!=1)
		{
			for (int i=1;i<=amount-1;i++)
			{
				ch->nxxtbro=va_arg(list,Treenode);
				ch=ch->nxxtbro;
			}
			ch->nxxtbro = NULL;
		}
		va_end(list);
	}
	return rt;
}

static void dfs(Treenode now, int depth)
{
	if (now==NULL||now->Token==NULL) return;
	if(strcmp (now->Token,"EMPTY"))
	{
	printf("%*s",2*depth,"");
	printf("%s",now->Token);
	if((!strcmp(now->Token,"ID"))||(!strcmp(now->Token,"TYPE")))
		printf(": %s",now->Id_Type);
	else if(!strcmp(now->Token,"INT"))
		printf(": %d",now->intval);
	else if(!strcmp(now->Token,"FLOAT"))
		printf(": %f",now->floatval);
	if(!(now->isleaf))
		printf(" (%d)",now->line);
	printf("\n");
}
	dfs(now->firstchild,depth+1);
	dfs(now->nxxtbro,depth);
}

#endif

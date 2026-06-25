LEX = flex
YACC = bison
CC = gcc

TARGET = parser
LEX_SRC = lexical.l
YACC_SRC = syntax.y

CFLAGS = -Wall -Wextra -std=gnu99

all: $(TARGET)

lex.yy.c: $(LEX_SRC) syntax.tab.h
	$(LEX) $(LEX_SRC)

syntax.tab.c syntax.tab.h: $(YACC_SRC)
	$(YACC) -d $(YACC_SRC)

$(TARGET): main.c treenode.c syntax.tab.c lex.yy.c treenode.h
	$(CC) $(CFLAGS) main.c treenode.c syntax.tab.c lex.yy.c -lfl -o $(TARGET)

clean:
	rm -f $(TARGET) lex.yy.c syntax.tab.c syntax.tab.h

.PHONY: all clean

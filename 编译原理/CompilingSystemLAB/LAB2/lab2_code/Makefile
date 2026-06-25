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

$(TARGET): main.c treenode.c semantic.c syntax.tab.c lex.yy.c treenode.h semantic.h
	$(CC) $(CFLAGS) main.c treenode.c semantic.c syntax.tab.c lex.yy.c -lfl -o $(TARGET)

clean:
	rm -f $(TARGET) lex.yy.c syntax.tab.c syntax.tab.h
	rm -f testcases/out_*.txt

test: $(TARGET)
	@for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17; do \
		./$(TARGET) testcases/required_$$i > testcases/out_$$i.txt; \
		diff -u testcases/expected/expected_$$i.txt testcases/out_$$i.txt || exit 1; \
	done
	@echo "All required semantic tests passed."

.PHONY: all clean test

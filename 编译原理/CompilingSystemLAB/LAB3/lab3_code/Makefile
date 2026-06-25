CC = gcc
FLEX = flex
BISON = bison

TARGET = parser
CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-function

OBJS = main.o Tree.o semantic.o intercode.o syntax.tab.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lfl

syntax.tab.c syntax.tab.h: syntax.y
	$(BISON) -d -v -o syntax.tab.c syntax.y

lex.yy.c: lexical.l syntax.tab.h
	$(FLEX) -o lex.yy.c lexical.l

syntax.tab.o: syntax.tab.c lex.yy.c
	$(CC) $(CFLAGS) -c syntax.tab.c -o syntax.tab.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) testcases/testcase_1 testcases/out1.ir
	./$(TARGET) testcases/testcase_2 testcases/out2.ir

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f testcases/out*.ir

.PHONY: all clean test

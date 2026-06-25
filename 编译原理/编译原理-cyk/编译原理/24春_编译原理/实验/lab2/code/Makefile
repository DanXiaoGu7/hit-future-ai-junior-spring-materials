# GNU make手册：http://www.gnu.org/software/make/manual/make.html
# ************ 遇到不明白的地方请google以及阅读手册 *************

# 编译器设定和编译选项
CC = gcc
FLEX = flex
BISON = bison
# CFLAGS = -std=c99

# 编译目标：src目录下的所有.c文件
CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

parser: syntax $(filter-out $(LFO),$(OBJS))
	$(CC) -o parser $(filter-out $(LFO),$(OBJS)) -lfl

syntax: lexical syntax-c
	$(CC) -c $(YFC) -o $(YFO)

lexical: $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

syntax-c: $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))

# 定义的一些伪目标
.PHONY: clean test1 test3 test5 test6 test7 test10 test12 test2 test4 test8 test9 test11 test13 test14 test15 test16 test17
test1:
	./parser ../test1.cmm
test3:
	./parser ../test3.cmm
test5:
	./parser ../test5.cmm
test6:
	./parser ../test6.cmm
test7:
	./parser ../test7.cmm
test10:
	./parser ../test10.cmm
test12:
	./parser ../test12.cmm
test2:
	./parser ../test2.cmm
test4:
	./parser ../test4.cmm
test8:
	./parser ../test8.cmm
test9:
	./parser ../test9.cmm
test11:
	./parser ../test11.cmm
test13:
	./parser ../test13.cmm
test14:
	./parser ../test14.cmm
test15:
	./parser ../test15.cmm
test16:
	./parser ../test16.cmm
test17:
	./parser ../test17.cmm
clean:
	rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(LFC) $(YFC) $(YFC:.c=.h)
	rm -f *~

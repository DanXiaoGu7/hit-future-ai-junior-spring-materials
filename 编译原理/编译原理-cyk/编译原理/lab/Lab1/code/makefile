# 变量定义
LEX_FILE = lexical2.l
BISON_FILE = syntax.y
C_FILE = main2.c
DEBUG_C_FILE = main_dbg.c
OUTPUT = parser

# 判断是否启用调试模式
ifdef DEBUG
    BISON_FLAGS = -d -t
    C_FILE = $(DEBUG_C_FILE)
else
    BISON_FLAGS = -d
endif

# 默认目标
all: $(OUTPUT)

# 生成词法分析器
lexical: $(LEX_FILE)
	flex $(LEX_FILE)

# 生成语法分析器
bison: $(BISON_FILE)
	bison $(BISON_FLAGS) $(BISON_FILE)

# 编译并生成最终可执行文件
$(OUTPUT): lexical bison $(C_FILE) syntax.tab.c
	gcc $(C_FILE) syntax.tab.c -lfl -o $(OUTPUT)

# 清理编译文件
clean:
	rm -f $(OUTPUT) lex.yy.c syntax.tab.c syntax.tab.h

# 启用调试模式
debug:
	$(MAKE) DEBUG=1

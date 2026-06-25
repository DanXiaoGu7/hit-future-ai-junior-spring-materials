/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 4 "./syntax.y"

    #include <stdarg.h>
    #include "lex.yy.c"
    Node* root = NULL;
    Node** package(int childNum, Node* child1, ...);
    void yyerror(const char* msg);
    int synError = 0;

#line 80 "./syntax.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "syntax.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_ID = 5,                         /* ID  */
  YYSYMBOL_SEMI = 6,                       /* SEMI  */
  YYSYMBOL_COMMA = 7,                      /* COMMA  */
  YYSYMBOL_ASSIGNOP = 8,                   /* ASSIGNOP  */
  YYSYMBOL_RELOP = 9,                      /* RELOP  */
  YYSYMBOL_PLUS = 10,                      /* PLUS  */
  YYSYMBOL_MINUS = 11,                     /* MINUS  */
  YYSYMBOL_STAR = 12,                      /* STAR  */
  YYSYMBOL_DIV = 13,                       /* DIV  */
  YYSYMBOL_AND = 14,                       /* AND  */
  YYSYMBOL_OR = 15,                        /* OR  */
  YYSYMBOL_DOT = 16,                       /* DOT  */
  YYSYMBOL_NOT = 17,                       /* NOT  */
  YYSYMBOL_TYPE = 18,                      /* TYPE  */
  YYSYMBOL_LP = 19,                        /* LP  */
  YYSYMBOL_RP = 20,                        /* RP  */
  YYSYMBOL_LB = 21,                        /* LB  */
  YYSYMBOL_RB = 22,                        /* RB  */
  YYSYMBOL_LC = 23,                        /* LC  */
  YYSYMBOL_RC = 24,                        /* RC  */
  YYSYMBOL_STRUCT = 25,                    /* STRUCT  */
  YYSYMBOL_RETURN = 26,                    /* RETURN  */
  YYSYMBOL_IF = 27,                        /* IF  */
  YYSYMBOL_ELSE = 28,                      /* ELSE  */
  YYSYMBOL_WHILE = 29,                     /* WHILE  */
  YYSYMBOL_LOWER_THAN_ELSE = 30,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_YYACCEPT = 31,                  /* $accept  */
  YYSYMBOL_Program = 32,                   /* Program  */
  YYSYMBOL_ExtDefList = 33,                /* ExtDefList  */
  YYSYMBOL_ExtDef = 34,                    /* ExtDef  */
  YYSYMBOL_ExtDecList = 35,                /* ExtDecList  */
  YYSYMBOL_Specifier = 36,                 /* Specifier  */
  YYSYMBOL_StructSpecifier = 37,           /* StructSpecifier  */
  YYSYMBOL_OptTag = 38,                    /* OptTag  */
  YYSYMBOL_Tag = 39,                       /* Tag  */
  YYSYMBOL_VarDec = 40,                    /* VarDec  */
  YYSYMBOL_FunDec = 41,                    /* FunDec  */
  YYSYMBOL_VarList = 42,                   /* VarList  */
  YYSYMBOL_ParamDec = 43,                  /* ParamDec  */
  YYSYMBOL_CompSt = 44,                    /* CompSt  */
  YYSYMBOL_StmtList = 45,                  /* StmtList  */
  YYSYMBOL_Stmt = 46,                      /* Stmt  */
  YYSYMBOL_DefList = 47,                   /* DefList  */
  YYSYMBOL_Def = 48,                       /* Def  */
  YYSYMBOL_DecList = 49,                   /* DecList  */
  YYSYMBOL_Dec = 50,                       /* Dec  */
  YYSYMBOL_Exp = 51,                       /* Exp  */
  YYSYMBOL_Args = 52                       /* Args  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   438

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  22
/* YYNRULES -- Number of rules.  */
#define YYNRULES  89
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  160

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   285


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    30,    30,    34,    36,    39,    41,    43,    45,    47,
      49,    51,    54,    56,    58,    63,    65,    68,    70,    72,
      74,    76,    78,    81,    83,    86,    91,    93,    95,    97,
     100,   102,   104,   106,   109,   111,   114,   119,   122,   124,
     127,   129,   131,   133,   135,   137,   139,   141,   143,   145,
     147,   152,   154,   157,   160,   162,   164,   167,   169,   171,
     176,   178,   180,   182,   184,   186,   188,   190,   192,   194,
     196,   198,   200,   202,   204,   206,   208,   210,   212,   214,
     216,   218,   220,   222,   224,   226,   228,   230,   233,   235
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "ID",
  "SEMI", "COMMA", "ASSIGNOP", "RELOP", "PLUS", "MINUS", "STAR", "DIV",
  "AND", "OR", "DOT", "NOT", "TYPE", "LP", "RP", "LB", "RB", "LC", "RC",
  "STRUCT", "RETURN", "IF", "ELSE", "WHILE", "LOWER_THAN_ELSE", "$accept",
  "Program", "ExtDefList", "ExtDef", "ExtDecList", "Specifier",
  "StructSpecifier", "OptTag", "Tag", "VarDec", "FunDec", "VarList",
  "ParamDec", "CompSt", "StmtList", "Stmt", "DefList", "Def", "DecList",
  "Dec", "Exp", "Args", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-69)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-55)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      90,     1,   -69,     3,    27,   -69,    90,   411,   -69,   -69,
      -8,    18,    26,   -69,   -69,   -69,    51,    61,   -69,    58,
      15,    53,    57,    54,   -69,   112,   -69,    77,    92,   106,
     -69,    57,   -69,     8,    78,    57,    86,    96,   105,   -69,
      92,   108,   109,    92,   -69,   -69,   120,   121,    39,   136,
      -2,   139,    45,   -69,   -69,   -69,   -69,   -69,   127,   -69,
      57,   -69,   -69,   -69,    42,   -69,   -69,   132,   258,   258,
     258,   258,   140,   145,   -69,   141,    39,   270,   258,   258,
     -69,     8,     8,   -69,   -69,   258,   130,   252,    65,   328,
     284,   135,   258,   -69,   -69,   -69,   152,   157,   174,   179,
     196,   201,   218,   223,   161,   240,   384,   384,   -69,   -69,
     342,   147,   -69,   299,   150,   -69,   -69,   153,   356,   370,
     -69,   384,   -69,   417,   -69,   252,   -69,   252,   -69,    65,
     -69,    65,   -69,   410,   -69,   397,   -69,   159,   313,    95,
     -69,   258,   -69,    66,   100,    95,   -69,   -69,   -69,   -69,
      95,   -69,    19,   144,   -69,   -69,    95,    95,   -69,   -69
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,    15,     0,     0,     2,     0,     0,    16,    10,
      22,    25,     0,    18,     1,     3,    11,    26,     6,     0,
       0,     0,    52,     0,     9,     0,     5,     0,     0,     0,
       7,    52,     8,     0,     0,    52,    21,     0,    33,    31,
       0,     0,    35,     0,    26,    13,    29,     0,     0,     0,
      57,     0,     0,    19,    51,    20,    17,    32,    36,    30,
       0,    14,    28,    27,     0,    76,    77,    75,     0,     0,
       0,     0,     0,     0,    41,     0,     0,     0,     0,     0,
      53,     0,     0,    34,    46,     0,     0,    69,    70,     0,
       0,     0,     0,    37,    38,    40,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,    58,    56,    55,
       0,     0,    72,    89,     0,    68,    42,     0,     0,     0,
      78,    60,    81,    63,    82,    64,    83,    65,    84,    66,
      85,    67,    79,    61,    80,    62,    74,     0,     0,     0,
      86,     0,    71,     0,     0,     0,    87,    73,    50,    88,
       0,    47,     0,    43,    45,    49,     0,     0,    48,    44
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -69,   -69,   180,   -69,    17,    14,   -69,   -69,   -69,   -28,
     -69,   128,   -69,   166,   113,   110,   215,   -69,     6,   -69,
     -68,    62
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     4,     5,     6,    19,    33,     8,    12,    13,    20,
      21,    41,    42,    74,    75,    76,    34,    35,    51,    52,
      77,   114
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      87,    88,    89,    90,    10,    50,    79,     9,    11,    49,
     106,   107,    58,    44,     7,    22,    27,   110,   113,    29,
       7,   -12,    28,   118,   119,    84,   -24,    14,   121,   123,
     125,   127,   129,   131,   133,   135,    29,   138,    85,    40,
      64,   -23,    65,    66,    67,    45,    81,   156,    84,    23,
      68,   -54,    82,    50,    50,    36,    69,    24,    70,    30,
      61,    85,    31,   -39,    26,    71,    72,    64,    73,    65,
      66,    67,     2,   113,    40,     2,    31,    68,   -52,     3,
      25,   104,     3,    69,    43,    70,   105,   108,   109,    31,
      -4,     1,    71,    72,   150,    73,    64,    44,    65,    66,
      67,   152,    53,    65,    66,    67,    68,    46,     2,    47,
      55,    68,    69,    38,    70,     3,    60,    69,    31,    70,
      56,    71,    72,    31,    73,    57,    71,    72,    59,    73,
       2,   111,    39,    65,    66,    67,   117,     3,    65,    66,
      67,    68,    62,    63,    78,    80,    68,    69,    29,    70,
     112,    86,    69,   120,    70,    65,    66,    67,   122,    91,
      65,    66,    67,    68,    92,    93,   136,   140,    68,    69,
     142,    70,   157,   143,    69,   124,    70,    65,    66,    67,
     126,   146,    65,    66,    67,    68,    15,    32,    83,    94,
      68,    69,     0,    70,     0,     0,    69,   128,    70,    65,
      66,    67,   130,   149,    65,    66,    67,    68,     0,     0,
       0,     0,    68,    69,     0,    70,     0,     0,    69,   132,
      70,    65,    66,    67,   134,     0,    65,    66,    67,    68,
       0,     0,     0,     0,    68,    69,     0,    70,    37,     0,
      69,   137,    70,    65,    66,    67,    48,     0,     0,   148,
      54,    68,     0,   151,   153,   154,     0,    69,     0,    70,
     155,    65,    66,    67,   100,   101,   158,   159,   104,    68,
       0,     0,     0,   105,     0,    69,    95,    70,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     0,     0,     0,
     116,   105,    96,    97,    98,    99,   100,   101,   102,   103,
     104,     0,     0,     0,     0,   105,   141,    96,    97,    98,
      99,   100,   101,   102,   103,   104,     0,     0,     0,     0,
     105,    96,    97,    98,    99,   100,   101,   102,   103,   104,
       0,     0,     0,     0,   105,   147,    96,    97,    98,    99,
     100,   101,   102,   103,   104,     0,     0,     0,   115,   105,
      96,    97,    98,    99,   100,   101,   102,   103,   104,     0,
       0,     0,   139,   105,    96,    97,    98,    99,   100,   101,
     102,   103,   104,     0,     0,     0,   144,   105,    96,    97,
      98,    99,   100,   101,   102,   103,   104,     0,     0,     0,
     145,   105,    96,    97,    98,    99,   100,   101,   102,   103,
     104,     0,     0,     0,     0,   105,    97,    98,    99,   100,
     101,   102,    16,   104,     0,     0,    17,    18,   105,    97,
      98,    99,   100,   101,     0,     0,   104,    98,    99,   100,
     101,   105,     0,   104,     0,     0,     0,     0,   105
};

static const yytype_int16 yycheck[] =
{
      68,    69,    70,    71,     1,    33,     8,     6,     5,     1,
      78,    79,    40,     5,     0,    23,     1,    85,    86,    21,
       6,     6,     7,    91,    92,     6,    23,     0,    96,    97,
      98,    99,   100,   101,   102,   103,    21,   105,    19,    25,
       1,    23,     3,     4,     5,    28,     1,    28,     6,    23,
      11,     6,     7,    81,    82,     1,    17,     6,    19,     6,
      43,    19,    23,    24,     6,    26,    27,     1,    29,     3,
       4,     5,    18,   141,    60,    18,    23,    11,    24,    25,
      19,    16,    25,    17,     7,    19,    21,    81,    82,    23,
       0,     1,    26,    27,    28,    29,     1,     5,     3,     4,
       5,     1,    24,     3,     4,     5,    11,     1,    18,     3,
      24,    11,    17,     1,    19,    25,     7,    17,    23,    19,
      24,    26,    27,    23,    29,    20,    26,    27,    20,    29,
      18,     1,    20,     3,     4,     5,     1,    25,     3,     4,
       5,    11,    22,    22,     8,     6,    11,    17,    21,    19,
      20,    19,    17,     1,    19,     3,     4,     5,     1,    19,
       3,     4,     5,    11,    19,    24,     5,    20,    11,    17,
      20,    19,    28,    20,    17,     1,    19,     3,     4,     5,
       1,    22,     3,     4,     5,    11,     6,    21,    60,    76,
      11,    17,    -1,    19,    -1,    -1,    17,     1,    19,     3,
       4,     5,     1,   141,     3,     4,     5,    11,    -1,    -1,
      -1,    -1,    11,    17,    -1,    19,    -1,    -1,    17,     1,
      19,     3,     4,     5,     1,    -1,     3,     4,     5,    11,
      -1,    -1,    -1,    -1,    11,    17,    -1,    19,    23,    -1,
      17,     1,    19,     3,     4,     5,    31,    -1,    -1,   139,
      35,    11,    -1,   143,   144,   145,    -1,    17,    -1,    19,
     150,     3,     4,     5,    12,    13,   156,   157,    16,    11,
      -1,    -1,    -1,    21,    -1,    17,     6,    19,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    -1,    -1,    -1,
       6,    21,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    -1,    -1,    -1,    -1,    21,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,
      21,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      -1,    -1,    -1,    -1,    21,    22,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    -1,    -1,    -1,    20,    21,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    -1,
      -1,    -1,    20,    21,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    -1,    -1,    -1,    20,    21,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    -1,    -1,    -1,
      20,    21,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    -1,    -1,    -1,    -1,    21,     9,    10,    11,    12,
      13,    14,     1,    16,    -1,    -1,     5,     6,    21,     9,
      10,    11,    12,    13,    -1,    -1,    16,    10,    11,    12,
      13,    21,    -1,    16,    -1,    -1,    -1,    -1,    21
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,    18,    25,    32,    33,    34,    36,    37,     6,
       1,     5,    38,    39,     0,    33,     1,     5,     6,    35,
      40,    41,    23,    23,     6,    19,     6,     1,     7,    21,
       6,    23,    44,    36,    47,    48,     1,    47,     1,    20,
      36,    42,    43,     7,     5,    35,     1,     3,    47,     1,
      40,    49,    50,    24,    47,    24,    24,    20,    40,    20,
       7,    35,    22,    22,     1,     3,     4,     5,    11,    17,
      19,    26,    27,    29,    44,    45,    46,    51,     8,     8,
       6,     1,     7,    42,     6,    19,    19,    51,    51,    51,
      51,    19,    19,    24,    45,     6,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    21,    51,    51,    49,    49,
      51,     1,    20,    51,    52,    20,     6,     1,    51,    51,
       1,    51,     1,    51,     1,    51,     1,    51,     1,    51,
       1,    51,     1,    51,     1,    51,     5,     1,    51,    20,
      20,     7,    20,    20,    20,    20,    22,    22,    46,    52,
      28,    46,     1,    46,    46,    46,    28,    28,    46,    46
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    31,    32,    33,    33,    34,    34,    34,    34,    34,
      34,    34,    35,    35,    35,    36,    36,    37,    37,    37,
      37,    37,    37,    38,    38,    39,    40,    40,    40,    40,
      41,    41,    41,    41,    42,    42,    43,    44,    45,    45,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    47,    47,    48,    49,    49,    49,    50,    50,    50,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    52,    52
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     0,     3,     2,     3,     3,     3,
       2,     2,     1,     3,     4,     1,     1,     5,     2,     5,
       5,     4,     2,     1,     0,     1,     1,     4,     4,     3,
       4,     3,     4,     3,     3,     1,     2,     4,     2,     0,
       2,     1,     3,     5,     7,     5,     2,     5,     7,     6,
       5,     2,     0,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     4,     3,     4,     3,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     4,     3,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* Program: ExtDefList  */
#line 30 "./syntax.y"
                                                { yyval = createNode("Program", ENUM_SYN_NOT_NULL, (yyloc).first_line, 
                                                  1, package(1, yyvsp[0]));
                                                  root = yyval; }
#line 1662 "./syntax.tab.c"
    break;

  case 3: /* ExtDefList: ExtDef ExtDefList  */
#line 34 "./syntax.y"
                                                { yyval = createNode("ExtDefList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1669 "./syntax.tab.c"
    break;

  case 4: /* ExtDefList: %empty  */
#line 36 "./syntax.y"
                                                { yyval = createNode("ExtDefList", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL);}
#line 1676 "./syntax.tab.c"
    break;

  case 5: /* ExtDef: Specifier ExtDecList SEMI  */
#line 39 "./syntax.y"
                                                { yyval = createNode("ExtDef", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1683 "./syntax.tab.c"
    break;

  case 6: /* ExtDef: Specifier SEMI  */
#line 41 "./syntax.y"
                                                { yyval = createNode("ExtDef", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1690 "./syntax.tab.c"
    break;

  case 7: /* ExtDef: Specifier FunDec SEMI  */
#line 43 "./syntax.y"
                                                { yyval = createNode("ExtDef", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1697 "./syntax.tab.c"
    break;

  case 8: /* ExtDef: Specifier FunDec CompSt  */
#line 45 "./syntax.y"
                                                { yyval = createNode("ExtDef", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1704 "./syntax.tab.c"
    break;

  case 9: /* ExtDef: Specifier error SEMI  */
#line 47 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1711 "./syntax.tab.c"
    break;

  case 10: /* ExtDef: error SEMI  */
#line 49 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1718 "./syntax.tab.c"
    break;

  case 11: /* ExtDef: Specifier error  */
#line 51 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1725 "./syntax.tab.c"
    break;

  case 12: /* ExtDecList: VarDec  */
#line 54 "./syntax.y"
                                                { yyval = createNode("ExtDecList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1732 "./syntax.tab.c"
    break;

  case 13: /* ExtDecList: VarDec COMMA ExtDecList  */
#line 56 "./syntax.y"
                                                { yyval = createNode("ExtDecList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1739 "./syntax.tab.c"
    break;

  case 14: /* ExtDecList: VarDec error COMMA ExtDecList  */
#line 58 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1746 "./syntax.tab.c"
    break;

  case 15: /* Specifier: TYPE  */
#line 63 "./syntax.y"
                                                { yyval = createNode("Specifier", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1753 "./syntax.tab.c"
    break;

  case 16: /* Specifier: StructSpecifier  */
#line 65 "./syntax.y"
                                                { yyval = createNode("Specifier", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1760 "./syntax.tab.c"
    break;

  case 17: /* StructSpecifier: STRUCT OptTag LC DefList RC  */
#line 68 "./syntax.y"
                                                { yyval = createNode("StructSpecifier", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 5, package(5, yyvsp[-4], yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1767 "./syntax.tab.c"
    break;

  case 18: /* StructSpecifier: STRUCT Tag  */
#line 70 "./syntax.y"
                                                { yyval = createNode("StructSpecifier", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1774 "./syntax.tab.c"
    break;

  case 19: /* StructSpecifier: STRUCT error LC DefList RC  */
#line 72 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1781 "./syntax.tab.c"
    break;

  case 20: /* StructSpecifier: STRUCT OptTag LC error RC  */
#line 74 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1788 "./syntax.tab.c"
    break;

  case 21: /* StructSpecifier: STRUCT OptTag LC error  */
#line 76 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1795 "./syntax.tab.c"
    break;

  case 22: /* StructSpecifier: STRUCT error  */
#line 78 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1802 "./syntax.tab.c"
    break;

  case 23: /* OptTag: ID  */
#line 81 "./syntax.y"
                                                { yyval = createNode("OptTag", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1809 "./syntax.tab.c"
    break;

  case 24: /* OptTag: %empty  */
#line 83 "./syntax.y"
                                                { yyval = createNode("OptTag", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); }
#line 1816 "./syntax.tab.c"
    break;

  case 25: /* Tag: ID  */
#line 86 "./syntax.y"
                                                { yyval = createNode("Tag", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1823 "./syntax.tab.c"
    break;

  case 26: /* VarDec: ID  */
#line 91 "./syntax.y"
                                                { yyval = createNode("VarDec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1830 "./syntax.tab.c"
    break;

  case 27: /* VarDec: VarDec LB INT RB  */
#line 93 "./syntax.y"
                                                { yyval = createNode("VarDec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 4, package(4, yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1837 "./syntax.tab.c"
    break;

  case 28: /* VarDec: VarDec LB error RB  */
#line 95 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1844 "./syntax.tab.c"
    break;

  case 29: /* VarDec: VarDec LB error  */
#line 97 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1851 "./syntax.tab.c"
    break;

  case 30: /* FunDec: ID LP VarList RP  */
#line 100 "./syntax.y"
                                                { yyval = createNode("FunDec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 4, package(4, yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1858 "./syntax.tab.c"
    break;

  case 31: /* FunDec: ID LP RP  */
#line 102 "./syntax.y"
                                                { yyval = createNode("FunDec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1865 "./syntax.tab.c"
    break;

  case 32: /* FunDec: ID LP error RP  */
#line 104 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1872 "./syntax.tab.c"
    break;

  case 33: /* FunDec: ID LP error  */
#line 106 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1879 "./syntax.tab.c"
    break;

  case 34: /* VarList: ParamDec COMMA VarList  */
#line 109 "./syntax.y"
                                                { yyval = createNode("VarList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1886 "./syntax.tab.c"
    break;

  case 35: /* VarList: ParamDec  */
#line 111 "./syntax.y"
                                                { yyval = createNode("VarList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1893 "./syntax.tab.c"
    break;

  case 36: /* ParamDec: Specifier VarDec  */
#line 114 "./syntax.y"
                                                { yyval = createNode("ParamDec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1900 "./syntax.tab.c"
    break;

  case 37: /* CompSt: LC DefList StmtList RC  */
#line 119 "./syntax.y"
                                                { yyval = createNode("CompSt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 4, package(4, yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1907 "./syntax.tab.c"
    break;

  case 38: /* StmtList: Stmt StmtList  */
#line 122 "./syntax.y"
                                                { yyval = createNode("StmtList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1914 "./syntax.tab.c"
    break;

  case 39: /* StmtList: %empty  */
#line 124 "./syntax.y"
                                                { yyval = createNode("StmtList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 0, NULL); }
#line 1921 "./syntax.tab.c"
    break;

  case 40: /* Stmt: Exp SEMI  */
#line 127 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 1928 "./syntax.tab.c"
    break;

  case 41: /* Stmt: CompSt  */
#line 129 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 1935 "./syntax.tab.c"
    break;

  case 42: /* Stmt: RETURN Exp SEMI  */
#line 131 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1942 "./syntax.tab.c"
    break;

  case 43: /* Stmt: IF LP Exp RP Stmt  */
#line 133 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 5, package(5, yyvsp[-4], yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1949 "./syntax.tab.c"
    break;

  case 44: /* Stmt: IF LP Exp RP Stmt ELSE Stmt  */
#line 135 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 7, package(7, yyvsp[-6], yyvsp[-5], yyvsp[-4], yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1956 "./syntax.tab.c"
    break;

  case 45: /* Stmt: WHILE LP Exp RP Stmt  */
#line 137 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 5, package(5, yyvsp[-4], yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 1963 "./syntax.tab.c"
    break;

  case 46: /* Stmt: error SEMI  */
#line 139 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1970 "./syntax.tab.c"
    break;

  case 47: /* Stmt: IF LP error RP Stmt  */
#line 141 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1977 "./syntax.tab.c"
    break;

  case 48: /* Stmt: IF LP Exp RP error ELSE Stmt  */
#line 143 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1984 "./syntax.tab.c"
    break;

  case 49: /* Stmt: IF LP error RP ELSE Stmt  */
#line 145 "./syntax.y"
                                            { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1991 "./syntax.tab.c"
    break;

  case 50: /* Stmt: error LP Exp RP Stmt  */
#line 147 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 1998 "./syntax.tab.c"
    break;

  case 51: /* DefList: Def DefList  */
#line 152 "./syntax.y"
                                                { yyval = createNode("DefList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 2005 "./syntax.tab.c"
    break;

  case 52: /* DefList: %empty  */
#line 154 "./syntax.y"
                                                { yyval = createNode("Stmt", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); }
#line 2012 "./syntax.tab.c"
    break;

  case 53: /* Def: Specifier DecList SEMI  */
#line 157 "./syntax.y"
                                                { yyval = createNode("Def", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2019 "./syntax.tab.c"
    break;

  case 54: /* DecList: Dec  */
#line 160 "./syntax.y"
                                                { yyval = createNode("DecList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2026 "./syntax.tab.c"
    break;

  case 55: /* DecList: Dec COMMA DecList  */
#line 162 "./syntax.y"
                                                { yyval = createNode("DecList", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2033 "./syntax.tab.c"
    break;

  case 56: /* DecList: Dec error DecList  */
#line 164 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2040 "./syntax.tab.c"
    break;

  case 57: /* Dec: VarDec  */
#line 167 "./syntax.y"
                                                { yyval = createNode("Dec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2047 "./syntax.tab.c"
    break;

  case 58: /* Dec: VarDec ASSIGNOP Exp  */
#line 169 "./syntax.y"
                                                { yyval = createNode("Dec", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2054 "./syntax.tab.c"
    break;

  case 59: /* Dec: error ASSIGNOP Exp  */
#line 171 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2061 "./syntax.tab.c"
    break;

  case 60: /* Exp: Exp ASSIGNOP Exp  */
#line 176 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2068 "./syntax.tab.c"
    break;

  case 61: /* Exp: Exp AND Exp  */
#line 178 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2075 "./syntax.tab.c"
    break;

  case 62: /* Exp: Exp OR Exp  */
#line 180 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2082 "./syntax.tab.c"
    break;

  case 63: /* Exp: Exp RELOP Exp  */
#line 182 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2089 "./syntax.tab.c"
    break;

  case 64: /* Exp: Exp PLUS Exp  */
#line 184 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2096 "./syntax.tab.c"
    break;

  case 65: /* Exp: Exp MINUS Exp  */
#line 186 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2103 "./syntax.tab.c"
    break;

  case 66: /* Exp: Exp STAR Exp  */
#line 188 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2110 "./syntax.tab.c"
    break;

  case 67: /* Exp: Exp DIV Exp  */
#line 190 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2117 "./syntax.tab.c"
    break;

  case 68: /* Exp: LP Exp RP  */
#line 192 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2124 "./syntax.tab.c"
    break;

  case 69: /* Exp: MINUS Exp  */
#line 194 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 2131 "./syntax.tab.c"
    break;

  case 70: /* Exp: NOT Exp  */
#line 196 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 2, package(2, yyvsp[-1], yyvsp[0])); }
#line 2138 "./syntax.tab.c"
    break;

  case 71: /* Exp: ID LP Args RP  */
#line 198 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 4, package(4, yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2145 "./syntax.tab.c"
    break;

  case 72: /* Exp: ID LP RP  */
#line 200 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2152 "./syntax.tab.c"
    break;

  case 73: /* Exp: Exp LB Exp RB  */
#line 202 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 4, package(4, yyvsp[-3], yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2159 "./syntax.tab.c"
    break;

  case 74: /* Exp: Exp DOT ID  */
#line 204 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2166 "./syntax.tab.c"
    break;

  case 75: /* Exp: ID  */
#line 206 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2173 "./syntax.tab.c"
    break;

  case 76: /* Exp: INT  */
#line 208 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2180 "./syntax.tab.c"
    break;

  case 77: /* Exp: FLOAT  */
#line 210 "./syntax.y"
                                                { yyval = createNode("Exp", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2187 "./syntax.tab.c"
    break;

  case 78: /* Exp: Exp ASSIGNOP error  */
#line 212 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2194 "./syntax.tab.c"
    break;

  case 79: /* Exp: Exp AND error  */
#line 214 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2201 "./syntax.tab.c"
    break;

  case 80: /* Exp: Exp OR error  */
#line 216 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2208 "./syntax.tab.c"
    break;

  case 81: /* Exp: Exp RELOP error  */
#line 218 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2215 "./syntax.tab.c"
    break;

  case 82: /* Exp: Exp PLUS error  */
#line 220 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2222 "./syntax.tab.c"
    break;

  case 83: /* Exp: Exp MINUS error  */
#line 222 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2229 "./syntax.tab.c"
    break;

  case 84: /* Exp: Exp STAR error  */
#line 224 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2236 "./syntax.tab.c"
    break;

  case 85: /* Exp: Exp DIV error  */
#line 226 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2243 "./syntax.tab.c"
    break;

  case 86: /* Exp: ID LP error RP  */
#line 228 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2250 "./syntax.tab.c"
    break;

  case 87: /* Exp: Exp LB error RB  */
#line 230 "./syntax.y"
                                                { yyval = createNode("Error", ENUM_SYN_NULL, (yyloc).first_line
                                                  , 0, NULL); yyerrok; }
#line 2257 "./syntax.tab.c"
    break;

  case 88: /* Args: Exp COMMA Args  */
#line 233 "./syntax.y"
                                                { yyval = createNode("Args", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 3, package(3, yyvsp[-2], yyvsp[-1], yyvsp[0])); }
#line 2264 "./syntax.tab.c"
    break;

  case 89: /* Args: Exp  */
#line 235 "./syntax.y"
                                                { yyval = createNode("Args", ENUM_SYN_NOT_NULL, (yyloc).first_line
                                                  , 1, package(1, yyvsp[0])); }
#line 2271 "./syntax.tab.c"
    break;


#line 2275 "./syntax.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 238 "./syntax.y"

Node** package(int childNum, Node* child1, ...) {
    va_list ap;
    va_start(ap, child1);
    Node** res = (Node**)malloc(sizeof(Node*) * childNum);
    res[0] = child1;
    for (int i = 1; i < childNum; i++)
    {
        res[i] = va_arg(ap, Node*);
    }
    return res;
}

void yyerror(const char* msg) { 
    synError++;
    printf("Error type B at Line %d: %s\n", yylineno, msg);
}

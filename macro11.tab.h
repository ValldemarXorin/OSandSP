
#ifndef YY_YY_MACRO11_TAB_H_INCLUDED
# define YY_YY_MACRO11_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 18 "macro11.y"
 #include "Ast.h" 

#line 52 "macro11.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INT = 258,                     /* INT  */
    FLOAT = 259,                   /* FLOAT  */
    STRING = 260,                  /* STRING  */
    COMMAND = 261,                 /* COMMAND  */
    REGISTER = 262,                /* REGISTER  */
    LABEL = 263,                   /* LABEL  */
    TOKEN_DIRECT_ASSIGN = 264,     /* "="  */
    TOKEN_TERM_INDICATOR = 265,    /* "%"  */
    TOKEN_IMMEDIATE_EXPR = 266,    /* "#"  */
    TOKEN_DEFFERRED_EXPR = 267,    /* "@"  */
    TOKEN_LEFT_BRACKET = 268,      /* "("  */
    TOKEN_RIGHT_BRACKET = 269,     /* ")"  */
    TOKEN_COMMA = 270,             /* ","  */
    TOKEN_PLUS = 271,              /* "+"  */
    TOKEN_MINUS = 272,             /* "-"  */
    TOKEN_MUL = 273,               /* "*"  */
    TOKEN_DIV = 274,               /* "/"  */
    TOKEN_LOGIC_AND = 275,         /* "&"  */
    TOKEN_LOGIC_OR = 276,          /* "!"  */
    DOLLAR = 277                   /* "$"  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 20 "macro11.y"

  int ival;
  float fval;
  char* sval;
  AST::LabelNode*   label;
  AST::OperandNode* operand;
  AST::CommandNode* command_line;
  AST::CommandNode* command_list;
  AST::ProgramNode* program;

#line 102 "macro11.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (AST::AbstractSyntaxTree* Ast);


#endif /* !YY_YY_MACRO11_TAB_H_INCLUDED  */

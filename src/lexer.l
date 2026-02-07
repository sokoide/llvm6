%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Forward declarations for types */
typedef struct ASTNode ASTNode;
typedef struct TypeInfo TypeInfo;
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR, OP_BITAND, OP_BITOR, OP_XOR,
    OP_LSHIFT, OP_RSHIFT,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN,
    OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
    OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
    OP_LSHIFT_ASSIGN, OP_RSHIFT_ASSIGN,
    OP_COMMA
} BinaryOp;

typedef enum {
    UOP_PLUS, UOP_MINUS, UOP_NOT, UOP_BITNOT,
    UOP_PREINC, UOP_PREDEC, UOP_POSTINC, UOP_POSTDEC,
    UOP_ADDR, UOP_DEREF, UOP_SIZEOF
} UnaryOp;

#include "grammar.tab.hpp"

/* Safe string duplication with error handling */
static int safe_strdup_to_yylval(const char* str) {
    char* result = strdup(str);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in lexer\n");
        return(0);  /* Trigger parse error */
    }
    yylval.str_val = result;
    return 1;  /* Success */
}

void count(void);
void comment(void);
int check_type(void);

int column = 0;
%}

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%option noyywrap

%%
"/*"			{ comment(); }

"auto"			{ count(); return(AUTO); }
"break"			{ count(); return(BREAK); }
"case"			{ count(); return(CASE); }
"char"			{ count(); return(CHAR); }
"const"			{ count(); return(CONST); }
"continue"		{ count(); return(CONTINUE); }
"default"		{ count(); return(DEFAULT); }
"do"			{ count(); return(DO); }
"double"		{ count(); return(DOUBLE); }
"else"			{ count(); return(ELSE); }
"enum"			{ count(); return(ENUM); }
"extern"		{ count(); return(EXTERN); }
"float"			{ count(); return(FLOAT); }
"for"			{ count(); return(FOR); }
"goto"			{ count(); return(GOTO); }
"if"			{ count(); return(IF); }
"int"			{ count(); return(INT); }
"long"			{ count(); return(LONG); }
"register"		{ count(); return(REGISTER); }
"return"		{ count(); return(RETURN); }
"short"			{ count(); return(SHORT); }
"signed"		{ count(); return(SIGNED); }
"sizeof"		{ count(); return(SIZEOF); }
"static"		{ count(); return(STATIC); }
"struct"		{ count(); return(STRUCT); }
"switch"		{ count(); return(SWITCH); }
"typedef"		{ count(); return(TYPEDEF); }
"union"			{ count(); return(UNION); }
"unsigned"		{ count(); return(UNSIGNED); }
"void"			{ count(); return(VOID); }
"volatile"		{ count(); return(VOLATILE); }
"while"			{ count(); return(WHILE); }

{L}({L}|{D})*		{ count(); safe_strdup_to_yylval(yytext); return(check_type()); }

0[xX]{H}+{IS}?		{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }
0{D}+{IS}?		{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }
{D}+{IS}?		{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }
L?'(\\.|[^\\'])+'	{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }

{D}+{E}{FS}?		{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }
{D}*"."{D}+({E})?{FS}?	{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }
{D}+"."{D}*({E})?{FS}?	{ count(); safe_strdup_to_yylval(yytext); return(CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ count(); safe_strdup_to_yylval(yytext); return(STRING_LITERAL); }

"..."			{ count(); return(ELLIPSIS); }
">>="			{ count(); return(RIGHT_ASSIGN); }
"<<="			{ count(); return(LEFT_ASSIGN); }
"+="			{ count(); return(ADD_ASSIGN); }
"-="			{ count(); return(SUB_ASSIGN); }
"*="			{ count(); return(MUL_ASSIGN); }
"/="			{ count(); return(DIV_ASSIGN); }
"%="			{ count(); return(MOD_ASSIGN); }
"&="			{ count(); return(AND_ASSIGN); }
"^="			{ count(); return(XOR_ASSIGN); }
"|="			{ count(); return(OR_ASSIGN); }
">>"			{ count(); return(RIGHT_OP); }
"<<"			{ count(); return(LEFT_OP); }
"++"			{ count(); return(INC_OP); }
"--"			{ count(); return(DEC_OP); }
"->"			{ count(); return(PTR_OP); }
"&&"			{ count(); return(AND_OP); }
"||"			{ count(); return(OR_OP); }
"<="			{ count(); return(LE_OP); }
">="			{ count(); return(GE_OP); }
"=="			{ count(); return(EQ_OP); }
"!="			{ count(); return(NE_OP); }
";"			{ count(); return(';'); }
("{"|"<%")		{ count(); return('{'); }
("}"|"%>")		{ count(); return('}'); }
","			{ count(); return(','); }
":"			{ count(); return(':'); }
"="			{ count(); return('='); }
"("			{ count(); return('('); }
")"			{ count(); return(')'); }
("["|"<:")		{ count(); return('['); }
("]"|":>")		{ count(); return(']'); }
"."			{ count(); return('.'); }
"&"			{ count(); return('&'); }
"!"			{ count(); return('!'); }
"~"			{ count(); return('~'); }
"-"			{ count(); return('-'); }
"+"			{ count(); return('+'); }
"*"			{ count(); return('*'); }
"/"			{ count(); return('/'); }
"%"			{ count(); return('%'); }
"<"			{ count(); return('<'); }
">"			{ count(); return('>'); }
"^"			{ count(); return('^'); }
"|"			{ count(); return('|'); }
"?"			{ count(); return('?'); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%

void comment(void)
{
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
		; /* consume character silently */

	if ((c1 = input()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}
}

void count(void)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}

int check_type(void)
{
	/*
	* pseudo code --- this is what it should check
	*
	* if (yytext == type_name)
	*	return(TYPE_NAME);
	*
	* return(IDENTIFIER);
	*/

	return(IDENTIFIER);
}
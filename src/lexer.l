%{
#include "ast.h"
#include "symbols.h"
#include "grammar_c.tab.h"

/* Safe string duplication with error handling */
static int safe_strdup_to_yylval(const char* str) {
    char* result = strdup(str);
    if (!result) {
        error_report("Memory allocation failed in lexer");
        return 0;
    }
    yylval.str_val = result;
    return 1;
}

void count(void);
void comment(void);
int check_type(void);
void skip_attribute(void);

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
"_Nullable"		{ count(); }
"_Nonnull"		{ count(); }
"__unused"		{ count(); }
"__deprecated"		{ count(); }
"__dead2"		{ count(); }
"__pure2"		{ count(); }
"__block"		{ count(); }
"__weak"		{ count(); }
"__strong"		{ count(); }
"__unsafe_unretained"	{ count(); }
"__restrict"		{ count(); return(CONST); }
"restrict"		{ count(); return(CONST); }
"__inline"		{ count(); }
"inline"		{ count(); }
"__attribute__"		{ count(); skip_attribute(); }
"__asm"			{ count(); skip_attribute(); }
"__asm__"		{ count(); skip_attribute(); }
"__extension__"		{ count(); }

"/*"			{ comment(); }
"//".*			{ count(); }
^#.*			{ count(); /* Skip preprocessor directives */ }

"auto"			{ count(); return(AUTO); }
"_Bool"			{ count(); return(BOOL); }
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

"__builtin_va_arg"	{ count(); return(SIZEOF); }
"__builtin_va_start"	{ count(); return(SIZEOF); }
"__builtin_va_end"	{ count(); return(SIZEOF); }
"__builtin_va_copy"	{ count(); return(SIZEOF); }
"__builtin_expect"	{ count(); return(SIZEOF); }

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
	Symbol* sym = symbol_lookup(yytext);
	if (sym && sym->type && sym->type->storage_class == STORAGE_TYPEDEF) {
		return(TYPE_NAME);
	}
	return(IDENTIFIER);
}

void skip_attribute(void) {
    int c;
    int parens = 0;
    while ((c = input()) != 0) {
        if (c == '(') parens++;
        else if (c == ')') {
            parens--;
            if (parens <= 0) break;
        }
    }
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int yylex(void);
extern int yyparse(void);
#ifdef __cplusplus
}
#endif
extern char* yytext;
extern int column;
extern FILE* yyin;

/* Global variables */
ASTNode* program_ast = NULL;
CodeGenContext* codegen_ctx = NULL;

/* Error handling */
#ifdef __cplusplus
extern "C" {
#endif
int yyerror(const char* s);
#ifdef __cplusplus
}
#endif
%}

%union {
    int int_val;
    float float_val;
    char* str_val;
    ASTNode* ast_node;
    TypeInfo* type_info;
    BinaryOp binary_op;
    UnaryOp unary_op;
}

%token <str_val> IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%type <ast_node> primary_expression postfix_expression argument_expression_list
%type <ast_node> unary_expression cast_expression multiplicative_expression
%type <ast_node> additive_expression shift_expression relational_expression
%type <ast_node> equality_expression and_expression exclusive_or_expression
%type <ast_node> inclusive_or_expression logical_and_expression logical_or_expression
%type <ast_node> conditional_expression assignment_expression expression
%type <ast_node> constant_expression declaration declaration_specifiers
%type <ast_node> init_declarator_list init_declarator declarator
%type <ast_node> direct_declarator pointer type_qualifier_list
%type <ast_node> parameter_type_list parameter_list parameter_declaration
%type <ast_node> identifier_list type_name abstract_declarator
%type <ast_node> direct_abstract_declarator initializer initializer_list
%type <ast_node> statement labeled_statement compound_statement
%type <ast_node> declaration_list statement_list expression_statement
%type <ast_node> selection_statement iteration_statement jump_statement
%type <ast_node> translation_unit external_declaration function_definition
%type <ast_node> struct_or_union_specifier struct_declaration_list
%type <ast_node> struct_declaration specifier_qualifier_list
%type <ast_node> struct_declarator_list struct_declarator enum_specifier
%type <ast_node> enumerator_list enumerator

%type <type_info> storage_class_specifier type_specifier type_qualifier
%type <binary_op> assignment_operator
%type <unary_op> unary_operator
%type <str_val> struct_or_union

%start translation_unit

%%

primary_expression
	: IDENTIFIER
		{ $$ = create_identifier_node($1); }
	| CONSTANT
		{ $$ = create_constant_node(atoi($1), TYPE_INT); }
	| STRING_LITERAL
		{ $$ = create_string_literal_node($1); }
	| '(' expression ')'
		{ $$ = $2; }
	;

postfix_expression
	: primary_expression
		{ $$ = $1; }
	| postfix_expression '[' expression ']'
		{ 
			$$ = create_ast_node(AST_ARRAY_ACCESS);
			$$->data.array_access.array = $1;
			$$->data.array_access.index = $3;
		}
	| postfix_expression '(' ')'
		{ $$ = create_function_call_node($1, NULL); }
	| postfix_expression '(' argument_expression_list ')'
		{ $$ = create_function_call_node($1, $3); }
	| postfix_expression '.' IDENTIFIER
		{
			$$ = create_ast_node(AST_MEMBER_ACCESS);
			$$->data.member_access.object = $1;
			$$->data.member_access.member = $3;
			$$->data.member_access.is_pointer_access = 0;
		}
	| postfix_expression PTR_OP IDENTIFIER
		{
			$$ = create_ast_node(AST_MEMBER_ACCESS);
			$$->data.member_access.object = $1;
			$$->data.member_access.member = $3;
			$$->data.member_access.is_pointer_access = 1;
		}
	| postfix_expression INC_OP
		{ $$ = create_unary_op_node(UOP_POSTINC, $1); }
	| postfix_expression DEC_OP
		{ $$ = create_unary_op_node(UOP_POSTDEC, $1); }
	;

argument_expression_list
	: assignment_expression
		{ $$ = $1; }
	| argument_expression_list ',' assignment_expression
		{
			$$ = $1;
			/* Add to argument list */
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $3;
		}
	;

unary_expression
	: postfix_expression
		{ $$ = $1; }
	| INC_OP unary_expression
		{ $$ = create_unary_op_node(UOP_PREINC, $2); }
	| DEC_OP unary_expression
		{ $$ = create_unary_op_node(UOP_PREDEC, $2); }
	| unary_operator cast_expression
		{ $$ = create_unary_op_node($1, $2); }
	| SIZEOF unary_expression
		{ $$ = create_unary_op_node(UOP_SIZEOF, $2); }
	| SIZEOF '(' type_name ')'
		{ $$ = create_unary_op_node(UOP_SIZEOF, $3); }
	;

unary_operator
	: '&'  { $$ = UOP_ADDR; }
	| '*'  { $$ = UOP_DEREF; }
	| '+'  { $$ = UOP_PLUS; }
	| '-'  { $$ = UOP_MINUS; }
	| '~'  { $$ = UOP_BITNOT; }
	| '!'  { $$ = UOP_NOT; }
	;

cast_expression
	: unary_expression
		{ $$ = $1; }
	| '(' type_name ')' cast_expression
		{
			$$ = create_ast_node(AST_CAST);
			/* Set cast type and operand */
		}
	;

multiplicative_expression
	: cast_expression
		{ $$ = $1; }
	| multiplicative_expression '*' cast_expression
		{ $$ = create_binary_op_node(OP_MUL, $1, $3); }
	| multiplicative_expression '/' cast_expression
		{ $$ = create_binary_op_node(OP_DIV, $1, $3); }
	| multiplicative_expression '%' cast_expression
		{ $$ = create_binary_op_node(OP_MOD, $1, $3); }
	;

additive_expression
	: multiplicative_expression
		{ $$ = $1; }
	| additive_expression '+' multiplicative_expression
		{ $$ = create_binary_op_node(OP_ADD, $1, $3); }
	| additive_expression '-' multiplicative_expression
		{ $$ = create_binary_op_node(OP_SUB, $1, $3); }
	;

shift_expression
	: additive_expression
		{ $$ = $1; }
	| shift_expression LEFT_OP additive_expression
		{ $$ = create_binary_op_node(OP_LSHIFT, $1, $3); }
	| shift_expression RIGHT_OP additive_expression
		{ $$ = create_binary_op_node(OP_RSHIFT, $1, $3); }
	;

relational_expression
	: shift_expression
		{ $$ = $1; }
	| relational_expression '<' shift_expression
		{ $$ = create_binary_op_node(OP_LT, $1, $3); }
	| relational_expression '>' shift_expression
		{ $$ = create_binary_op_node(OP_GT, $1, $3); }
	| relational_expression LE_OP shift_expression
		{ $$ = create_binary_op_node(OP_LE, $1, $3); }
	| relational_expression GE_OP shift_expression
		{ $$ = create_binary_op_node(OP_GE, $1, $3); }
	;

equality_expression
	: relational_expression
		{ $$ = $1; }
	| equality_expression EQ_OP relational_expression
		{ $$ = create_binary_op_node(OP_EQ, $1, $3); }
	| equality_expression NE_OP relational_expression
		{ $$ = create_binary_op_node(OP_NE, $1, $3); }
	;

and_expression
	: equality_expression
		{ $$ = $1; }
	| and_expression '&' equality_expression
		{ $$ = create_binary_op_node(OP_BITAND, $1, $3); }
	;

exclusive_or_expression
	: and_expression
		{ $$ = $1; }
	| exclusive_or_expression '^' and_expression
		{ $$ = create_binary_op_node(OP_XOR, $1, $3); }
	;

inclusive_or_expression
	: exclusive_or_expression
		{ $$ = $1; }
	| inclusive_or_expression '|' exclusive_or_expression
		{ $$ = create_binary_op_node(OP_BITOR, $1, $3); }
	;

logical_and_expression
	: inclusive_or_expression
		{ $$ = $1; }
	| logical_and_expression AND_OP inclusive_or_expression
		{ $$ = create_binary_op_node(OP_AND, $1, $3); }
	;

logical_or_expression
	: logical_and_expression
		{ $$ = $1; }
	| logical_or_expression OR_OP logical_and_expression
		{ $$ = create_binary_op_node(OP_OR, $1, $3); }
	;

conditional_expression
	: logical_or_expression
		{ $$ = $1; }
	| logical_or_expression '?' expression ':' conditional_expression
		{
			$$ = create_ast_node(AST_CONDITIONAL);
			/* Set conditional parts */
		}
	;

assignment_expression
	: conditional_expression
		{ $$ = $1; }
	| unary_expression assignment_operator assignment_expression
		{ $$ = create_binary_op_node($2, $1, $3); }
	;

assignment_operator
	: '='           { $$ = OP_ASSIGN; }
	| MUL_ASSIGN    { $$ = OP_MUL_ASSIGN; }
	| DIV_ASSIGN    { $$ = OP_DIV_ASSIGN; }
	| MOD_ASSIGN    { $$ = OP_MOD_ASSIGN; }
	| ADD_ASSIGN    { $$ = OP_ADD_ASSIGN; }
	| SUB_ASSIGN    { $$ = OP_SUB_ASSIGN; }
	| LEFT_ASSIGN   { $$ = OP_LSHIFT_ASSIGN; }
	| RIGHT_ASSIGN  { $$ = OP_RSHIFT_ASSIGN; }
	| AND_ASSIGN    { $$ = OP_AND_ASSIGN; }
	| XOR_ASSIGN    { $$ = OP_XOR_ASSIGN; }
	| OR_ASSIGN     { $$ = OP_OR_ASSIGN; }
	;

expression
	: assignment_expression
		{ $$ = $1; }
	| expression ',' assignment_expression
		{ $$ = create_binary_op_node(OP_COMMA, $1, $3); }
	;

constant_expression
	: conditional_expression
		{ $$ = $1; }
	;

declaration
	: declaration_specifiers ';'
		{ $$ = NULL; /* Empty declaration */ }
	| declaration_specifiers init_declarator_list ';'
		{ $$ = $2; /* Return declarator list */ }
	;

declaration_specifiers
	: storage_class_specifier
		{ $$ = NULL; /* Handle storage class */ }
	| storage_class_specifier declaration_specifiers
		{ $$ = $2; }
	| type_specifier
		{ $$ = NULL; /* Handle type specifier */ }
	| type_specifier declaration_specifiers
		{ $$ = $2; }
	| type_qualifier
		{ $$ = NULL; /* Handle type qualifier */ }
	| type_qualifier declaration_specifiers
		{ $$ = $2; }
	;

init_declarator_list
	: init_declarator
		{ $$ = $1; }
	| init_declarator_list ',' init_declarator
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $3;
		}
	;

init_declarator
	: declarator
		{ $$ = create_variable_decl_node(create_type_info(TYPE_INT), $1->data.identifier.name, NULL); }
	| declarator '=' initializer
		{ $$ = create_variable_decl_node(create_type_info(TYPE_INT), $1->data.identifier.name, $3); }
	;

storage_class_specifier
	: TYPEDEF   { $$ = NULL; }
	| EXTERN    { $$ = NULL; }
	| STATIC    { $$ = NULL; }
	| AUTO      { $$ = NULL; }
	| REGISTER  { $$ = NULL; }
	;

type_specifier
	: VOID      { $$ = create_type_info(TYPE_VOID); }
	| CHAR      { $$ = create_type_info(TYPE_CHAR); }
	| SHORT     { $$ = create_type_info(TYPE_SHORT); }
	| INT       { $$ = create_type_info(TYPE_INT); }
	| LONG      { $$ = create_type_info(TYPE_LONG); }
	| FLOAT     { $$ = create_type_info(TYPE_FLOAT); }
	| DOUBLE    { $$ = create_type_info(TYPE_DOUBLE); }
	| SIGNED    { $$ = create_type_info(TYPE_SIGNED); }
	| UNSIGNED  { $$ = create_type_info(TYPE_UNSIGNED); }
	| struct_or_union_specifier { $$ = NULL; }
	| enum_specifier            { $$ = NULL; }
	| TYPE_NAME                 { $$ = NULL; }
	;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'
		{ $$ = NULL; }
	| struct_or_union '{' struct_declaration_list '}'
		{ $$ = NULL; }
	| struct_or_union IDENTIFIER
		{ $$ = NULL; }
	;

struct_or_union
	: STRUCT  { $$ = "struct"; }
	| UNION   { $$ = "union"; }
	;

struct_declaration_list
	: struct_declaration
		{ $$ = $1; }
	| struct_declaration_list struct_declaration
		{ $$ = $1; /* Chain declarations */ }
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'
		{ $$ = $2; }
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
		{ $$ = NULL; /* Combine type specifiers */ }
	| type_specifier
		{ $$ = NULL; /* Single type specifier */ }
	| type_qualifier specifier_qualifier_list
		{ $$ = NULL; /* Type qualifier with list */ }
	| type_qualifier
		{ $$ = NULL; /* Single type qualifier */ }
	;

struct_declarator_list
	: struct_declarator
		{ $$ = $1; }
	| struct_declarator_list ',' struct_declarator
		{ $$ = $1; }
	;

struct_declarator
	: declarator
		{ $$ = $1; }
	| ':' constant_expression
		{ $$ = NULL; }
	| declarator ':' constant_expression
		{ $$ = $1; }
	;

enum_specifier
	: ENUM '{' enumerator_list '}'
		{ $$ = NULL; }
	| ENUM IDENTIFIER '{' enumerator_list '}'
		{ $$ = NULL; }
	| ENUM IDENTIFIER
		{ $$ = NULL; }
	;

enumerator_list
	: enumerator
		{ $$ = $1; }
	| enumerator_list ',' enumerator
		{ $$ = $1; }
	;

enumerator
	: IDENTIFIER
		{ $$ = create_identifier_node($1); }
	| IDENTIFIER '=' constant_expression
		{ $$ = create_identifier_node($1); }
	;

type_qualifier
	: CONST     { $$ = NULL; }
	| VOLATILE  { $$ = NULL; }
	;

declarator
	: pointer direct_declarator
		{ $$ = $2; }
	| direct_declarator
		{ $$ = $1; }
	;

direct_declarator
	: IDENTIFIER
		{ $$ = create_identifier_node($1); }
	| '(' declarator ')'
		{ $$ = $2; }
	| direct_declarator '[' constant_expression ']'
		{ $$ = $1; }
	| direct_declarator '[' ']'
		{ $$ = $1; }
	| direct_declarator '(' parameter_type_list ')'
		{ 
			$$ = $1; 
			$$->data.identifier.parameters = $3;
		}
	| direct_declarator '(' identifier_list ')'
		{ $$ = $1; }
	| direct_declarator '(' ')'
		{ $$ = $1; }
	;

pointer
	: '*'
		{ $$ = NULL; }
	| '*' type_qualifier_list
		{ $$ = NULL; }
	| '*' pointer
		{ $$ = NULL; }
	| '*' type_qualifier_list pointer
		{ $$ = NULL; }
	;

type_qualifier_list
	: type_qualifier
		{ $$ = NULL; }
	| type_qualifier_list type_qualifier
		{ $$ = NULL; }
	;

parameter_type_list
	: parameter_list
		{ $$ = $1; }
	| parameter_list ',' ELLIPSIS
		{ $$ = $1; }
	;

parameter_list
	: parameter_declaration
		{ $$ = $1; }
	| parameter_list ',' parameter_declaration
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $3;
		}
	;

parameter_declaration
	: declaration_specifiers declarator
		{ $$ = create_variable_decl_node(create_type_info(TYPE_INT), $2->data.identifier.name, NULL); }
	| declaration_specifiers abstract_declarator
		{ $$ = create_variable_decl_node(create_type_info(TYPE_INT), "param", NULL); }
	| declaration_specifiers
		{ $$ = create_variable_decl_node(create_type_info(TYPE_INT), "param", NULL); }
	;

identifier_list
	: IDENTIFIER
		{ $$ = create_identifier_node($1); }
	| identifier_list ',' IDENTIFIER
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = create_identifier_node($3);
		}
	;

type_name
	: specifier_qualifier_list
		{ $$ = NULL; }
	| specifier_qualifier_list abstract_declarator
		{ $$ = NULL; }
	;

abstract_declarator
	: pointer
		{ $$ = NULL; }
	| direct_abstract_declarator
		{ $$ = $1; }
	| pointer direct_abstract_declarator
		{ $$ = $2; }
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
		{ $$ = $2; }
	| '[' ']'
		{ $$ = NULL; }
	| '[' constant_expression ']'
		{ $$ = NULL; }
	| direct_abstract_declarator '[' ']'
		{ $$ = $1; }
	| direct_abstract_declarator '[' constant_expression ']'
		{ $$ = $1; }
	| '(' ')'
		{ $$ = NULL; }
	| '(' parameter_type_list ')'
		{ $$ = $2; }
	| direct_abstract_declarator '(' ')'
		{ $$ = $1; }
	| direct_abstract_declarator '(' parameter_type_list ')'
		{ $$ = $1; }
	;

initializer
	: assignment_expression
		{ $$ = $1; }
	| '{' initializer_list '}'
		{ $$ = $2; }
	| '{' initializer_list ',' '}'
		{ $$ = $2; }
	;

initializer_list
	: initializer
		{ $$ = $1; }
	| initializer_list ',' initializer
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $3;
		}
	;

statement
	: labeled_statement     { $$ = $1; }
	| compound_statement    { $$ = $1; }
	| expression_statement  { $$ = $1; }
	| selection_statement   { $$ = $1; }
	| iteration_statement   { $$ = $1; }
	| jump_statement        { $$ = $1; }
	;

labeled_statement
	: IDENTIFIER ':' statement
		{
			$$ = create_ast_node(AST_LABEL_STMT);
			/* Set label and statement */
		}
	| CASE constant_expression ':' statement
		{
			$$ = create_ast_node(AST_CASE_STMT);
			/* Set case expression and statement */
		}
	| DEFAULT ':' statement
		{
			$$ = create_ast_node(AST_DEFAULT_STMT);
			/* Set default statement */
		}
	;

compound_statement
	: '{' '}'
		{ $$ = create_compound_stmt_node(NULL); }
	| '{' statement_list '}'
		{ $$ = create_compound_stmt_node($2); }
	| '{' declaration_list '}'
		{ $$ = create_compound_stmt_node($2); }
	| '{' declaration_list statement_list '}'
		{
			/* Combine declarations and statements */
			if ($2) {
				$$ = create_compound_stmt_node($2);
				/* Chain the statement list to the end of declarations */
				ASTNode* current = $2;
				while (current->next) current = current->next;
				current->next = $3;
			} else {
				$$ = create_compound_stmt_node($3);
			}
		}
	;

declaration_list
	: declaration
		{ $$ = $1; }
	| declaration_list declaration
		{
			if ($1) {
				$$ = $1;
				ASTNode* current = $1;
				while (current->next) current = current->next;
				current->next = $2;
			} else {
				$$ = $2;
			}
		}
	;

statement_list
	: statement
		{ $$ = $1; }
	| statement_list statement
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $2;
		}
	;

expression_statement
	: ';'
		{ 
			$$ = create_ast_node(AST_EXPRESSION_STMT);
			$$->data.return_stmt.expression = NULL; /* Reuse return_stmt structure for expression */
		}
	| expression ';'
		{ 
			$$ = create_ast_node(AST_EXPRESSION_STMT);
			$$->data.return_stmt.expression = $1; /* Store the expression */
		}
	;

selection_statement
	: IF '(' expression ')' statement
		{ $$ = create_if_stmt_node($3, $5, NULL); }
	| IF '(' expression ')' statement ELSE statement
		{ $$ = create_if_stmt_node($3, $5, $7); }
	| SWITCH '(' expression ')' statement
		{
			$$ = create_ast_node(AST_SWITCH_STMT);
			/* Set switch expression and statement */
		}
	;

iteration_statement
	: WHILE '(' expression ')' statement
		{ $$ = create_while_stmt_node($3, $5); }
	| DO statement WHILE '(' expression ')' ';'
		{
			$$ = create_ast_node(AST_DO_WHILE_STMT);
			$$->data.while_stmt.condition = $5;
			$$->data.while_stmt.body = $2;
		}
	| FOR '(' expression_statement expression_statement ')' statement
		{ $$ = create_for_stmt_node($3, $4, NULL, $6); }
	| FOR '(' expression_statement expression_statement expression ')' statement
		{ $$ = create_for_stmt_node($3, $4, $5, $7); }
	;

jump_statement
	: GOTO IDENTIFIER ';'
		{
			$$ = create_ast_node(AST_GOTO_STMT);
			/* Set target label */
		}
	| CONTINUE ';'
		{ $$ = create_ast_node(AST_CONTINUE_STMT); }
	| BREAK ';'
		{ $$ = create_ast_node(AST_BREAK_STMT); }
	| RETURN ';'
		{ $$ = create_return_stmt_node(NULL); }
	| RETURN expression ';'
		{ $$ = create_return_stmt_node($2); }
	;

translation_unit
	: external_declaration
		{ 
			program_ast = $1;
			$$ = program_ast;
		}
	| translation_unit external_declaration
		{
			$$ = $1;
			ASTNode* current = $1;
			while (current->next) current = current->next;
			current->next = $2;
			program_ast = $$;
		}
	;

external_declaration
	: function_definition
		{ $$ = $1; }
	| declaration
		{ $$ = $1; }
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement
		{ $$ = create_function_def_node(create_type_info(TYPE_INT), $2->data.identifier.name, $3, $4); }
	| declaration_specifiers declarator compound_statement
		{ 
			/* Check if declarator has parameters (modern C syntax) */
			ASTNode* params = ($2->data.identifier.parameters) ? $2->data.identifier.parameters : NULL;
			$$ = create_function_def_node(create_type_info(TYPE_INT), $2->data.identifier.name, params, $3); 
		}
	| declarator declaration_list compound_statement
		{ $$ = create_function_def_node(create_type_info(TYPE_INT), $1->data.identifier.name, $2, $3); }
	| declarator compound_statement
		{ $$ = create_function_def_node(create_type_info(TYPE_INT), $1->data.identifier.name, NULL, $2); }
	;

%%

#ifdef __cplusplus
extern "C" {
#endif

int yyerror(const char* s) {
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);
	return 0;
}

#ifdef __cplusplus
}
#endif
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylex(void);
extern char* yytext;
extern int column;
extern FILE* yyin;

/* Global variables */
ASTNode* program_ast = NULL;

/* Error handling */
int yyerror(const char* s);
%}

/* Token precedence and associativity */
/* Resolve dangling-else ambiguity by making ELSE right-associative */
%right ELSE

/* Expect 1 shift/reduce conflict due to dangling-else ambiguity */
%expect 1

%union {
    int int_val;
    float float_val;
    char* str_val;
    ASTNode* ast_node;
    TypeInfo* type_info;
    BinaryOp binary_op;
    UnaryOp unary_op;
    struct {
        ASTNode* head;
        int is_variadic;
    } param_info;
}

%token <str_val> IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS BOOL

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

/* Resolve dangling-else ambiguity by making ELSE right-associative */
%type <ast_node> primary_expression postfix_expression argument_expression_list
%type <ast_node> unary_expression cast_expression multiplicative_expression
%type <ast_node> additive_expression shift_expression relational_expression
%type <ast_node> equality_expression and_expression exclusive_or_expression
%type <ast_node> inclusive_or_expression logical_and_expression logical_or_expression
%type <ast_node> conditional_expression assignment_expression expression
%type <ast_node> constant_expression declaration
%type <ast_node> init_declarator_list init_declarator declarator
%type <ast_node> block_item block_item_list
%type <ast_node> direct_declarator type_qualifier_list
%type <int_val> pointer
%type <ast_node> parameter_list parameter_declaration
%type <param_info> parameter_type_list
%type <ast_node> identifier_list abstract_declarator
%type <type_info> type_name
%type <ast_node> direct_abstract_declarator initializer initializer_list
%type <ast_node> statement labeled_statement compound_statement
%type <ast_node> declaration_list expression_statement
%type <ast_node> selection_statement iteration_statement jump_statement
%type <ast_node> translation_unit external_declaration function_definition
%type <ast_node> struct_or_union_specifier struct_declaration_list
%type <ast_node> struct_declaration
%type <ast_node> struct_declarator_list struct_declarator enum_specifier
%type <ast_node> enumerator_list enumerator

%type <type_info> storage_class_specifier type_specifier type_qualifier declaration_specifiers specifier_qualifier_list
%type <binary_op> assignment_operator
%type <unary_op> unary_operator
%type <str_val> struct_or_union

%start translation_unit

%%

primary_expression
	: IDENTIFIER
		{ $$ = create_identifier_node($1); }
	| CONSTANT
		{ $$ = create_constant_node(parse_constant_value($1), TYPE_INT); }
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
		{
			/* Create a constant node with the size of the type */
			int size = 4; /* Default size for most types */
			if ($3) {
				switch ($3->base_type) {
					case TYPE_CHAR: size = 1; break;
					case TYPE_SHORT: size = 2; break;
					case TYPE_INT: size = 4; break;
					case TYPE_LONG: size = 8; break;
					case TYPE_FLOAT: size = 4; break;
					case TYPE_DOUBLE: size = 8; break;
					case TYPE_POINTER: size = 8; break; /* 64-bit pointers */
					default: size = 4; break;
				}
			}
			$$ = create_constant_node(size, TYPE_INT);
		}
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
			$$->data.cast_expr.target_type = $2;
			$$->data.cast_expr.operand = $4;
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
			$$->data.conditional_expr.condition = $1;
			$$->data.conditional_expr.then_expr = $3;
			$$->data.conditional_expr.else_expr = $5;
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
		{ $$ = NULL; /* Empty declaration */ free_type_info($1); }
	| declaration_specifiers init_declarator_list ';'
		{
			$$ = $2;
			ASTNode* curr = $$;
			while (curr) {
				TypeInfo* full_type = duplicate_type_info($1);
				int p_level = 0;
				if (curr->type == AST_VARIABLE_DECL) {
					p_level = curr->data.variable_decl.pointer_level;
					for (int i = 0; i < p_level; i++) {
						full_type = create_pointer_type(full_type);
					}
					curr->data.variable_decl.type = full_type;
				} else if (curr->type == AST_FUNCTION_DECL) {
					p_level = curr->data.function_def.pointer_level;
					for (int i = 0; i < p_level; i++) {
						full_type = create_pointer_type(full_type);
					}
					curr->data.function_def.return_type = full_type;
				}
				curr = curr->next;
			}
			free_type_info($1);
		}
	;

declaration_specifiers
	: storage_class_specifier
		{ $$ = $1; }
	| storage_class_specifier declaration_specifiers
		{
			$$ = $2;
			$$->storage_class = $1->storage_class;
			free_type_info($1);
		}
	| type_specifier
		{ $$ = $1; }
	| type_specifier declaration_specifiers
		{
			$$ = $2;
			$$->base_type = $1->base_type;
			free_type_info($1);
		}
	| type_qualifier
		{ $$ = $1; }
	| type_qualifier declaration_specifiers
		{
			$$ = $2;
			$$->qualifiers = (TypeQualifier)($$->qualifiers | $1->qualifiers);
			free_type_info($1);
		}
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
		{
			if ($1->data.identifier.parameters) {
				$$ = create_function_decl_node(NULL, $1->data.identifier.name, $1->data.identifier.parameters, $1->data.identifier.is_variadic);
				$$->data.function_def.pointer_level = $1->data.identifier.pointer_level;
			} else {
				$$ = create_variable_decl_node(NULL, $1->data.identifier.name, NULL);
				$$->data.variable_decl.pointer_level = $1->data.identifier.pointer_level;
				$$->data.variable_decl.array_dimensions = $1->data.identifier.array_dimensions;
			}
		}
	| declarator '=' initializer
		{
			$$ = create_variable_decl_node(NULL, $1->data.identifier.name, $3);
			$$->data.variable_decl.pointer_level = $1->data.identifier.pointer_level;
			$$->data.variable_decl.array_dimensions = $1->data.identifier.array_dimensions;
		}
	;

storage_class_specifier
	: TYPEDEF   { $$ = create_type_info(TYPE_VOID); $$->storage_class = STORAGE_TYPEDEF; }
	| EXTERN    { $$ = create_type_info(TYPE_VOID); $$->storage_class = STORAGE_EXTERN; }
	| STATIC    { $$ = create_type_info(TYPE_VOID); $$->storage_class = STORAGE_STATIC; }
	| AUTO      { $$ = create_type_info(TYPE_VOID); $$->storage_class = STORAGE_AUTO; }
	| REGISTER  { $$ = create_type_info(TYPE_VOID); $$->storage_class = STORAGE_REGISTER; }
	;

type_specifier
	: VOID      { $$ = create_type_info(TYPE_VOID); }
	| BOOL      { $$ = create_type_info(TYPE_BOOL); }
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
	: STRUCT  { $$ = (char*)"struct"; }
	| UNION   { $$ = (char*)"union"; }
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
		{
			$$ = $2;
			$$->base_type = $1->base_type;
			free_type_info($1);
		}
	| type_specifier
		{ $$ = $1; }
	| type_qualifier specifier_qualifier_list
		{
			$$ = $2;
			$$->qualifiers = (TypeQualifier)($$->qualifiers | $1->qualifiers);
			free_type_info($1);
		}
	| type_qualifier
		{ $$ = $1; }
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
	: CONST     { $$ = create_type_info(TYPE_VOID); $$->qualifiers = QUAL_CONST; }
	| VOLATILE  { $$ = create_type_info(TYPE_VOID); $$->qualifiers = QUAL_VOLATILE; }
	;

declarator
	: pointer direct_declarator
		{
			$$ = $2;
			$$->data.identifier.pointer_level = $1;
		}
	| direct_declarator
		{
			$$ = $1;
			$$->data.identifier.pointer_level = 0;
		}
	;

direct_declarator
	: IDENTIFIER
		{
			$$ = create_identifier_node($1);
			$$->data.identifier.is_variadic = 0;
			$$->data.identifier.pointer_level = 0;
		}
	| '(' declarator ')'
		{ $$ = $2; }
	| direct_declarator '[' constant_expression ']'
		{
			$$ = $1;
			/* Prepend array dimension to dimension list */
			if ($3 && $3->type == AST_CONSTANT) {
				$3->next = $$->data.identifier.array_dimensions;
				$$->data.identifier.array_dimensions = $3;
			} else {
				ASTNode* zero = create_constant_node(0, TYPE_INT);
				zero->next = $$->data.identifier.array_dimensions;
				$$->data.identifier.array_dimensions = zero;
			}
		}
	| direct_declarator '[' ']'
		{
			$$ = $1;
			ASTNode* zero = create_constant_node(0, TYPE_INT);
			zero->next = $$->data.identifier.array_dimensions;
			$$->data.identifier.array_dimensions = zero;
		}
	| direct_declarator '(' parameter_type_list ')'
		{
			$$ = $1;
			$$->data.identifier.parameters = $3.head;
			$$->data.identifier.is_variadic = $3.is_variadic;
		}
	| direct_declarator '(' identifier_list ')'
		{
			$$ = $1;
			$$->data.identifier.is_variadic = 0;
		}
	| direct_declarator '(' ')'
		{
			$$ = $1;
			$$->data.identifier.is_variadic = 0;
		}
	;

pointer
	: '*'
		{ $$ = 1; }
	| '*' type_qualifier_list
		{ $$ = 1; }
	| '*' pointer
		{ $$ = $2 + 1; }
	| '*' type_qualifier_list pointer
		{ $$ = $3 + 1; }
	;

type_qualifier_list
	: type_qualifier
		{ $$ = NULL; }
	| type_qualifier_list type_qualifier
		{ $$ = NULL; }
	;

parameter_type_list
	: parameter_list
		{
			$$.head = $1;
			$$.is_variadic = 0;
		}
	| parameter_list ',' ELLIPSIS
		{
			$$.head = $1;
			$$.is_variadic = 1;
		}
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
		{
			TypeInfo* full_type = $1;
			for (int i = 0; i < $2->data.identifier.pointer_level; i++) {
				full_type = create_pointer_type(full_type);
			}
			$$ = create_variable_decl_node(full_type, $2->data.identifier.name, NULL);
		}
	| declaration_specifiers abstract_declarator
		{ $$ = create_variable_decl_node($1, (char*)"param", NULL); }
	| declaration_specifiers
		{ $$ = create_variable_decl_node($1, (char*)"param", NULL); }
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
		{ $$ = $1; }
	| specifier_qualifier_list abstract_declarator
		{ $$ = $1; }
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
		{ $$ = $2.head; }
	| direct_abstract_declarator '(' ')'
		{ $$ = $1; }
	| direct_abstract_declarator '(' parameter_type_list ')'
		{ $$ = $1; }
	;

initializer
	: assignment_expression
		{ $$ = $1; }
	| '{' initializer_list '}'
		{
			$$ = create_ast_node(AST_INITIALIZER_LIST);
			$$->data.initializer_list.items = $2;
			int count = 0;
			ASTNode* current = $2;
			while (current) { count++; current = current->next; }
			$$->data.initializer_list.count = count;
		}
	| '{' initializer_list ',' '}'
		{
			$$ = create_ast_node(AST_INITIALIZER_LIST);
			$$->data.initializer_list.items = $2;
			int count = 0;
			ASTNode* current = $2;
			while (current) { count++; current = current->next; }
			$$->data.initializer_list.count = count;
		}
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
			$$->data.case_stmt.value = $2;
			$$->data.case_stmt.statement = $4;
		}
	| DEFAULT ':' statement
		{
			$$ = create_ast_node(AST_DEFAULT_STMT);
			$$->data.case_stmt.value = NULL;  /* no value for default */
			$$->data.case_stmt.statement = $3;
		}
	;

compound_statement
	: '{' '}'
		{ $$ = create_compound_stmt_node(NULL); }
	| '{' block_item_list '}'
		{ $$ = create_compound_stmt_node($2); }
	;

block_item_list
	: block_item
		{ $$ = $1; }
	| block_item_list block_item
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

block_item
	: declaration
		{ $$ = $1; }
	| statement
		{ $$ = $1; }
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
			$$->data.switch_stmt.expression = $3;
			$$->data.switch_stmt.body = $5;
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
	| FOR '(' declaration expression_statement ')' statement
		{
			/* C99: for (int i = 0; condition; ) body */
			$$ = create_ast_node(AST_FOR_STMT);
			$$->data.for_stmt.init = $3;
			$$->data.for_stmt.condition = $4;
			$$->data.for_stmt.update = NULL;
			$$->data.for_stmt.body = $6;
		}
	| FOR '(' declaration expression_statement expression ')' statement
		{
			/* C99: for (int i = 0; condition; update) body */
			$$ = create_ast_node(AST_FOR_STMT);
			$$->data.for_stmt.init = $3;
			$$->data.for_stmt.condition = $4;
			$$->data.for_stmt.update = $5;
			$$->data.for_stmt.body = $7;
		}
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
		{ $$ = create_function_def_node($1, $2->data.identifier.name, $3, $4, $2->data.identifier.is_variadic); }
	| declaration_specifiers declarator compound_statement
		{
			/* Check if declarator has parameters (modern C syntax) */
			ASTNode* params = ($2->data.identifier.parameters) ? $2->data.identifier.parameters : NULL;
			$$ = create_function_def_node($1, $2->data.identifier.name, params, $3, $2->data.identifier.is_variadic);
		}
	| declarator declaration_list compound_statement
		{ $$ = create_function_def_node(create_type_info(TYPE_INT), $1->data.identifier.name, $2, $3, $1->data.identifier.is_variadic); }
	| declarator compound_statement
		{ $$ = create_function_def_node(create_type_info(TYPE_INT), $1->data.identifier.name, NULL, $2, $1->data.identifier.is_variadic); }
	;

%%

int yyerror(const char* s) {

	fflush(stdout);

	printf("\n%*s\n%*s\n", column, "^", column, s);

	return 0;

}



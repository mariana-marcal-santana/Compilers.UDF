%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <sstream>
#include <string>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!
#define NIL (new cdk::nil_node(LINE))
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;        /* expression type */
  //-- don't change *any* of these --- END!

  int                  i;           /* integer value */
  double               d;           /* real value */
  std::string          *s;          /* symbol name or string literal */
  std::size_t           s_t;        /* size_t value */
  std::vector<std::size_t> *dims;   /* dimensions for tensors */
  cdk::basic_node      *node;       /* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;

  udf::block_node *block;
};


%token tAND tOR tNE tLE tGE tSIZEOF 
%token tINPUT tWRITE tWRITELN tOBJECTS tCONTRACTION tRANK tCAPACITY tDIMS tDIM tRESHAPE
%token tPUBLIC tFORWARD tPRIVATE
%token tTYPE_STRING tTYPE_INT tTYPE_REAL tTYPE_POINTER tTYPE_AUTO tTYPE_VOID tTYPE_TENSOR
%token tIF tELIF tELSE
%token tFOR
%token tBREAK tCONTINUE tRETURN

%token<i> tINTEGER
%token<d> tREAL
%token <s> tSTRING tID
%token<expression> tNULLPTR
%type<dims> dims

%type <node> declaration vardec fundec fundef argdec instruction conditional_instruction  return else fordec
%type <sequence> file declarations vardecs opt_vardecs argdecs instructions opt_instructions opt_expressions expressions fordecs opt_forinit tensor_items tensor_item exprs_no_tensor
%type <expression> expression expr_no_tensor tensor
%type <lvalue> lval
%type <type> data_type void_type
%type <block> block
%type<s> string

%nonassoc tIFX
%nonassoc tELIF tELSE
%right '='
%left tOR
%left tAND
%nonassoc '?'
%nonassoc '~'
%left tEQ tNE
%left tGE tLE '>' '<'
%left '+' '-'
%left '*' '/' '%'
%left tCONTRACTION 
%left '.'
%nonassoc tRANK tCAPACITY tDIMS tDIM tRESHAPE '@'
%nonassoc tUNARY
%nonassoc '(' '['


%{
//-- The rules below will be included in yyparse, the main parsing function.
%}
%%

// file
file : /* empty */  { compiler->ast($$ = new cdk::sequence_node(LINE)); }
     | declarations { compiler->ast($$ = $1); }
     ;

// declarations
declarations :              declaration { $$ = new cdk::sequence_node(LINE, $1);     }
             | declarations declaration { $$ = new cdk::sequence_node(LINE, $2, $1); }
             ;

declaration  : vardec ';' { $$ = $1; }
             | fundec     { $$ = $1; }
             | fundef     { $$ = $1; }
             ;

// variables
vardec : tPUBLIC  data_type tID '=' expression       { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, $2, *$3, $5); delete $3; }
       | tPUBLIC  data_type tID                      { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, $2, *$3, nullptr); delete $3; }
       | tFORWARD data_type tID                      { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, $2, *$3, nullptr); delete $3; }
       | tPUBLIC tTYPE_AUTO tID '=' expression       { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, nullptr, *$3, $5); delete $3; }
       |          data_type tID '=' expression       { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, $4); delete $2; }
       |          data_type tID                      { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); delete $2; }
       |          tTYPE_AUTO tID '=' expression      { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); delete $2; }
       ;

vardecs      : vardec ';'          { $$ = new cdk::sequence_node(LINE, $1);     }
             | vardecs vardec ';'  { $$ = new cdk::sequence_node(LINE, $2, $1); }
             ;
             
opt_vardecs  : /* empty */ { $$ = NULL; }
             | vardecs     { $$ = $1; }
             ;

// functions
fundec   :          data_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4); }
         | tFORWARD data_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  $2, *$3, $5); }
         | tPUBLIC  data_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  $2, *$3, $5); }
         |          tTYPE_AUTO tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); }
         | tFORWARD tTYPE_AUTO tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  nullptr, *$3, $5); }
         | tPUBLIC  tTYPE_AUTO tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  nullptr, *$3, $5); }
         |          void_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4); }
         | tFORWARD void_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  $2, *$3, $5); }
         | tPUBLIC  void_type  tID '(' argdecs ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC,  $2, *$3, $5); }
         ;

fundef   :         data_type  tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6); }
         | tPUBLIC data_type  tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC,  $2, *$3, $5, $7); }
         |         tTYPE_AUTO tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, nullptr, *$2, $4, $6); }
         | tPUBLIC tTYPE_AUTO tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC,  nullptr, *$3, $5, $7); }
         |         void_type  tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6); }
         | tPUBLIC void_type  tID '(' argdecs ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC,  $2, *$3, $5, $7); }
         ;

// types
data_type    : tTYPE_STRING                     { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING);  }
             | tTYPE_INT                        { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT);     }
             | tTYPE_REAL                       { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE);  }
             | tTYPE_POINTER '<' data_type '>'  { $$ = cdk::reference_type::create(4, $3); }
             | tTYPE_POINTER '<' tTYPE_AUTO '>' { $$ = cdk::reference_type::create(4, nullptr); }
             | tTYPE_TENSOR '<' dims '>'        { $$ = cdk::tensor_type::create(*$3); }
             ;

dims : tINTEGER { $$ = new std::vector<size_t>(); $$->push_back($1); }
     | dims ',' tINTEGER         { $$ = $1; $1->push_back($3); }
     ;

void_type : tTYPE_VOID { $$ = cdk::primitive_type::create(LINE, cdk::TYPE_VOID); }
          ;

// block
block    : '{' opt_vardecs opt_instructions '}' { $$ = new udf::block_node(LINE, $2, $3); }
         ;

// block - arguments
argdecs  : /* empty */         { $$ = new cdk::sequence_node(LINE);  }
         |             argdec  { $$ = new cdk::sequence_node(LINE, $1);     }
         | argdecs ',' argdec  { $$ = new cdk::sequence_node(LINE, $3, $1); }
         ;

argdec   : data_type tID { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); }
         ;

// block - instructions
instructions    : instruction                { $$ = new cdk::sequence_node(LINE, $1);     }
                | instructions instruction   { $$ = new cdk::sequence_node(LINE, $2, $1); }
                ;

opt_instructions: /* empty */  { $$ = new cdk::sequence_node(LINE); }
                | instructions { $$ = $1; }
                ;

instruction     : conditional_instruction                                                       { $$ = $1; }
                | tFOR '(' opt_forinit ';' opt_expressions ';' opt_expressions ')' instruction  { $$ = new udf::for_node(LINE, $3, $5, $7, $9); }
                | expression ';'                                                                { $$ = new udf::evaluation_node(LINE, $1); }
                | tWRITE   expressions ';'                                                      { $$ = new udf::write_node(LINE, $2, false); }
                | tWRITELN expressions ';'                                                      { $$ = new udf::write_node(LINE, $2, true); }
                | tBREAK                                                                        { $$ = new udf::break_node(LINE);  }
                | tCONTINUE                                                                     { $$ = new udf::continue_node(LINE); }
                | return                                                                        { $$ = $1; }
                | block                                                                         { $$ = $1; }
                ;

// instructions - return
return          : tRETURN             ';' { $$ = new udf::return_node(LINE, nullptr); }
                | tRETURN expression  ';' { $$ = new udf::return_node(LINE, $2); }
                ;

// instructions - conditional
conditional_instruction : tIF '(' expression ')' instruction %prec tIFX         { $$ = new udf::if_node(LINE, $3, $5); }
                        | tIF '(' expression ')' instruction else               { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
                        ;

else : tELSE instruction                                                        { $$ = $2; }
     | tELIF '(' expression ')' instruction  %prec tIFX                         { $$ = new udf::if_node(LINE, $3, $5); }
     | tELIF '(' expression ')' instruction else                                { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
     ;

// instructions - iteration
fordec          : data_type tID '=' expression    { $$ = new udf::variable_declaration_node(LINE, tPRIVATE,  $1, *$2, $4); }
                | tID '=' expression              { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, nullptr, *$1, $3); delete $1; }
                | tID                             { $$ = new cdk::variable_node(LINE, $1); }
                ;
              
fordecs         :             fordec { $$ = new cdk::sequence_node(LINE, $1);     }
                | fordecs ',' fordec { $$ = new cdk::sequence_node(LINE, $3, $1); }
                ;

opt_forinit     : /**/     { $$ = new cdk::sequence_node(LINE, NIL); }
                | fordecs  { $$ = $1; }
                ;

// expressions
expr_no_tensor : tINTEGER              { $$ = new cdk::integer_node(LINE, $1); }
           | tREAL                 { $$ = new cdk::double_node(LINE, $1); };
           | string                { $$ = new cdk::string_node(LINE, $1); }
           | tNULLPTR              { $$ = new udf::nullptr_node(LINE); }
           /* LEFT VALUES */
           | lval                  { $$ = new cdk::rvalue_node(LINE, $1); }
           /* ASSIGNMENTS */
           | lval '=' expression         { $$ = new cdk::assignment_node(LINE, $1, $3); }
           /* ARITHMETIC EXPRESSIONS */
           | expression '+' expression         { $$ = new cdk::add_node(LINE, $1, $3); }
           | expression '-' expression         { $$ = new cdk::sub_node(LINE, $1, $3); }
           | expression '*' expression         { $$ = new cdk::mul_node(LINE, $1, $3); }
           | expression '/' expression         { $$ = new cdk::div_node(LINE, $1, $3); }
           | expression '%' expression         { $$ = new cdk::mod_node(LINE, $1, $3); }
           /* LOGICAL EXPRESSIONS */
           | expression '<' expression         { $$ = new cdk::lt_node(LINE, $1, $3); }
           | expression '>' expression         { $$ = new cdk::gt_node(LINE, $1, $3); }
           | expression tGE expression         { $$ = new cdk::ge_node(LINE, $1, $3); }
           | expression tLE expression         { $$ = new cdk::le_node(LINE, $1, $3); }
           | expression tNE expression         { $$ = new cdk::ne_node(LINE, $1, $3); }
           | expression tEQ expression         { $$ = new cdk::eq_node(LINE, $1, $3); }
           | expression tAND  expression       { $$ = new cdk::and_node(LINE, $1, $3); }
           | expression tOR   expression       { $$ = new cdk::or_node (LINE, $1, $3); }
            /* UNARY EXPRESSION */
           | '-' expression %prec tUNARY     { $$ = new cdk::unary_minus_node(LINE, $2); }
           | '+' expression %prec tUNARY     { $$ = new cdk::unary_plus_node(LINE, $2); }
           | '~' expression                  { $$ = new cdk::not_node(LINE, $2); }
           /* OTHER EXPRESSIONS */
           | '(' expression ')'              { $$ = $2; }
           | tID '(' opt_expressions ')'     { $$ = new udf::function_call_node(LINE, *$1, $3); delete $1; }
           | tSIZEOF '(' expression ')'      { $$ = new udf::sizeof_node(LINE, $3); }
           | tINPUT                          { $$ = new udf::input_node(LINE); }
           | tOBJECTS '(' expression ')'     { $$ = new udf::stack_alloc_node(LINE, $3); }
           | lval '?'                        { $$ = new udf::address_of_node(LINE, $1); }
           /* TENSOR EXPRESSIONS */
           | expression '.' tRANK                        { $$ = new udf::tensor_rank_node(LINE, $1); }
           | expression '.' tCAPACITY                    { $$ = new udf::tensor_capacity_node(LINE, $1); }
           | expression '.' tDIMS                        { $$ = new udf::tensor_dims_node(LINE, $1); }
           | expression '.' tDIM '(' expression ')'      { $$ = new udf::tensor_dim_node(LINE, $1, $5); } 
           ;


exprs_no_tensor : expr_no_tensor                         { $$ = new cdk::sequence_node(LINE, $1);     }      
                | exprs_no_tensor ',' expr_no_tensor      { $$ = new cdk::sequence_node(LINE, $3, $1); }
                ;

tensor_item : '[' exprs_no_tensor ']'                { $$ = $2; }
            | '[' tensor_items ']'                   { $$ = $2; }
            ;


tensor_items   : tensor_item                          { $$ = new cdk::sequence_node(LINE, $1);     }                     
               | tensor_items ',' tensor_item         { $$ = new cdk::sequence_node(LINE, $3, $1); }
               ;
               
tensor    :  tensor_item                                 {  $$ = new udf::tensor_node(LINE, $1); }
          |  expression '.' tRESHAPE '(' expressions ')' { $$ = new udf::tensor_reshape_node(LINE, $1, $5); }
          |  expression tCONTRACTION expression          { $$ = new udf::tensor_contraction_node(LINE, $1, $3); }
          ;

expression : expr_no_tensor                           { $$ = $1; } 
           | tensor                                   { $$ = $1; } 
           ;

expressions     : expression                     { $$ = new cdk::sequence_node(LINE, $1);     }
                | expressions ',' expression     { $$ = new cdk::sequence_node(LINE, $3, $1); }
                ;

opt_expressions : /* empty */         { $$ = new cdk::sequence_node(LINE); }
                | expressions         { $$ = $1; }
                ;

lval : tID                                { $$ = new cdk::variable_node(LINE, $1); }
     | expression '[' expression ']'      { $$ = new udf::index_node(LINE, $1, $3); }
     | expression '@' '(' expressions ')' { $$ = new udf::tensor_index_node(LINE, $1, $4); }
     ;
     
string          : tSTRING                       { $$ = $1; }
                | string tSTRING                { $$ = $1; $$->append(*$2); delete $2; }
                ;

%%
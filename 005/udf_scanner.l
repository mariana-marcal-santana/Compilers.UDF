%option c++ prefix="udf_scanner_" outfile="udf_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <string>
#include <errno.h>
#include <limits>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "udf_parser.tab.h"

// don't change this
#define yyerror LexerError
  
std::ostringstream strlit;

#define CHECK_REAL { \
  try { \
    yylval.d = std::stod(yytext); \
    return tREAL; \
  } catch (const std::out_of_range&) { \
    yyerror("real overflow"); \
  } \
}

%}
%x X_COMMENT X_STRING X_BACKSLASH X_HEX_INT X_ASCII_NULL

DIGIT        [0-9]
LETTER       [A-Za-z]
IDCHAR       [A-Za-z0-9_]
IDENTIFIER   {LETTER}{IDCHAR}*
INT          {DIGIT}+
EXP          [Ee][+-]?[0-9]+

%%
  /*yydebug=1; set_debug(1);*/

  /* ====================================================================== */
  /* ====[                      2.2 - COMMENTS                        ]==== */
  /* ====================================================================== */

"//".*                 ; /* ignore comments */
"/*"                   yy_push_state(X_COMMENT);
<X_COMMENT>"/*"        yy_push_state(X_COMMENT);
<X_COMMENT>"*/"        yy_pop_state();
<X_COMMENT>.|"\n"      ;  /* ignore inside comments */

  /* ====================================================================== */
  /* ====[                  1.1   - Tipos de dados                    ]==== */
  /* ====[                  1.1   - Null pointers                     ]==== */
  /* ====================================================================== */

"auto"                   return tTYPE_AUTO;
"int"                    return tTYPE_INT;
"real"                   return tTYPE_REAL;
"string"                 return tTYPE_STRING;
"ptr"                    return tTYPE_POINTER;
"nullptr"                return tNULLPTR;
"sizeof"                 return tSIZEOF;
"void"                   return tTYPE_VOID; 
[<>]                     return *yytext;
"tensor"                 return tTYPE_TENSOR;

  /* ====================================================================== */
  /* ====[                  3.4.2 - Símbolos globais                  ]==== */
  /* ====================================================================== */

"public"                 return tPUBLIC;
"forward"                return tFORWARD;

  /* ====================================================================== */
  /* ====[                    Funções Tensores                        ]==== */
  /* ====================================================================== */

  "rank"                   return tRANK;
  "capacity"               return tCAPACITY;
  "dims"                   return tDIMS;
  "dim"                    return tDIM;
  "reshape"                return tRESHAPE;

  /* ====================================================================== */
  /* ====[                 5.5 - Instrução condicional                ]==== */
  /* ====================================================================== */

"if"                   return tIF; 
"elif"                 return tELIF;
"else"                 return tELSE;

  /* ====================================================================== */
  /* ====[                 5.5 - Instrução de iteração                ]==== */
  /* ====[                 5.5 - Instrução de terminação              ]==== */
  /* ====[                 5.4 - Instrução de continuação             ]==== */
  /* ====================================================================== */

"for"                   return tFOR;
"break"                 return tBREAK;
"continue"              return tCONTINUE;

  /* ====================================================================== */
  /* ====[                 6 - Operadores de expressões               ]==== */
  /* ====================================================================== */

">="                   return tGE; 
"<="                   return tLE; 
"=="                   return tEQ; 
"!="                   return tNE;
"&&"                   return tAND;
"||"                   return tOR;
"**"                   return tCONTRACTION;
"input"                return tINPUT;
"objects"              return tOBJECTS;

  /* ====================================================================== */
  /* ====[              3   - Delimitadores e separadores             ]==== */
  /* ====[              4.3 - Corpo (de função)                       ]==== */
  /* ====[                 5.3 - Instrução de retorno                 ]==== */
  /* ====================================================================== */

"return"                       return tRETURN;
"write"                        return tWRITE;
"writeln"                      return tWRITELN;

  /* ====================================================================== */
  /* ====[                   2.7.1 - inteiros                         ]==== */
  /* ====================================================================== */

0x[[:xdigit:]]+ {
    errno = 0;
    unsigned long val = strtoul(yytext + 2, NULL, 16);  // ignore "0x"
    if (errno == ERANGE || val > std::numeric_limits<int>::max()) {
        yyerror("integer overflow");
    } else {
        yylval.i = val;
        return tINTEGER;
    }
}

0x[[:alnum:]]+ {
    yyerror("bad hexadecimal number");
    exit(1);
}

0x {
    yyerror("bad hexadecimal number");
    exit(1);
}

{INT}                  {
  errno = 0;
  unsigned long val = strtoul(yytext, NULL, 10);
  if (errno == ERANGE || val > std::numeric_limits<int>::max()) {
    yyerror("integer overflow");
  }
  else {
    yylval.i = val;
    return tINTEGER;
  }
}

  /* ====================================================================== */
  /* ====[                   2.7.2 - reais                            ]==== */
  /* ====================================================================== */

[0-9]*\.[0-9]+{EXP}?   CHECK_REAL;
[0-9]+\.[0-9]*{EXP}?   CHECK_REAL;
[0-9]+{EXP}            CHECK_REAL;
  

  /* ====================================================================== */
  /* ====[                2.6 - Identificadores (nomes)               ]==== */
  /* ====================================================================== */

{IDENTIFIER}  yylval.s = new std::string(yytext); return tID;

  /* ====================================================================== */
  /* ====[              2.7.3 - Cadeias de caracteres                 ]==== */
  /* ====================================================================== */

"\""                           yy_push_state(X_STRING);
<X_STRING>\\                   yy_push_state(X_BACKSLASH);
<X_STRING>"\"" {
  yylval.s = new std::string(strlit.str());
  strlit.str("");
  yy_pop_state();
  return tSTRING;
}
<X_STRING>.                    strlit << *yytext;

<X_BACKSLASH>n                 strlit << '\n'; yy_pop_state();
<X_BACKSLASH>r                 strlit << '\r'; yy_pop_state();
<X_BACKSLASH>t                 strlit << '\t'; yy_pop_state();
<X_BACKSLASH>\\                strlit << '\\'; yy_pop_state();
<X_BACKSLASH>"\""              strlit << '\"'; yy_pop_state();
<X_BACKSLASH>0[[:xdigit:]]     {
  strlit << (char)(unsigned char)strtoul(yytext, NULL, 16);
  yy_pop_state();
  }
<X_BACKSLASH>0"\""              {
  yylval.s = new std::string(strlit.str());
  strlit.str("");
  yy_pop_state();
  yy_pop_state();
  return tSTRING;
}
<X_BACKSLASH>0[^[:xdigit:]]     yy_push_state(X_ASCII_NULL);

<X_BACKSLASH>[[:xdigit:]]{1,2} {
  strlit << (char)(unsigned char)strtoul(yytext, NULL, 16);
  yy_pop_state();
}

<X_ASCII_NULL>"\""          {
  yylval.s = new std::string(strlit.str());
  strlit.str("");
  yy_pop_state();
  yy_pop_state();
  yy_pop_state();
  return tSTRING;
}
<X_ASCII_NULL>.|\n            ;


  /* ====================================================================== */
  /* ====[                    (other characters)                      ]==== */
  /* ====================================================================== */

[-()<>=+*/%;:{},\[\]@?~#.]      return *yytext;
[[:space:]]                     ; /* ignore remaining white space inside the program */
.                               yyerror((std::string("Unknown character: ") + yytext).c_str());

%%

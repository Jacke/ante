%{
#include <stdlib.h>
#include <stdio.h>
#include <tokens.h>
#include <ptree.h>

#ifndef YYSTYPE
#define YYSTYPE Node*
#endif

/* This has no effect when generating a c++ parser */
/* Setting verbose for a c++ parser requires %error-verbose, set in the next section */
#define YYERROR_VERBOSE

#include "yyparser.h"

/* Defined in lexer.cpp */
extern int yylex(yy::parser::semantic_type*, yy::location*);
extern Node* rootNode;
/*namespace ante{
    extern void error(string& msg, const char *fileName, unsigned int row, unsigned int col);
}*/

void yyerror(const char *msg);

%}

%locations
%error-verbose

%token Ident UserType

/* types */
%token I8 I16 I32 I64 
%token U8 U16 U32 U64
%token Isz Usz F16 F32 F64
%token C8 C32 Bool Void

/* operators */
%token Eq NotEq AddEq SubEq MulEq DivEq GrtrEq LesrEq
%token Or And Range Returns

/* literals */
%token True False
%token IntLit FltLit StrLit

/* keywords */
%token Return
%token If Then Elif Else
%token For While Do In
%token Continue Break
%token Import Let Var Match With
%token Data Enum Fun Ext

/* modifiers */
%token Pub Pri Pro Raw
%token Const Noinit Pathogen

/* other */
%token Where Infect Cleanse Ct

/* whitespace */
%token Newline Indent Unindent


/*
    Now to manually fix all shift/reduce conflicts
*/

/*
    Fake precedence rule to allow for a lower precedence
    than Ident in decl context
*/
%nonassoc LOW

%left Ident

%left ';' Newline
%left If
%left Let In Else Import Return
%left MED

%left ','
%left '=' AddEq SubEq MulEq DivEq

%left Or
%left And     
%left Eq  NotEq GrtrEq LesrEq '<' '>'

%left Range
%left '+' '-'
%left '*' '/' '%'

%left '.'
%left '@' '&'

/* 
    Being below HIGH, this ensures parenthetical expressions will be parsed
    as just order-of operations parenthesis, instead of a single-value tuple.
*/
%nonassoc ')'

%nonassoc '(' '[' Indent Unindent
%nonassoc HIGH

/*
    Expect 4 shift/reduce warnings, all from type casting.  Using a glr-parser
    resolves this ambiguity.
*/
%glr-parser
%expect 4
%start top_level_expr_list
%%

top_level_expr_list:  maybe_newline expr {rootNode = $2;}
                   ;



maybe_newline: Newline  %prec Newline
             | %empty
             ;


import_expr: Import expr {$$ = mkImportNode(@$, $2);}


ident: Ident {$$ = (Node*)lextxt;}
     ;

usertype: UserType {$$ = (Node*)lextxt;}
        ;

intlit: IntLit {$$ = mkIntLitNode(@$, lextxt);}
      ;

fltlit: FltLit {$$ = mkFltLitNode(@$, lextxt);}
      ;

strlit: StrLit {$$ = mkStrLitNode(@$, lextxt);}
      ;

lit_type: I8                        {$$ = mkTypeNode(@$, TT_I8,  (char*)"");}
        | I16                       {$$ = mkTypeNode(@$, TT_I16, (char*)"");}
        | I32                       {$$ = mkTypeNode(@$, TT_I32, (char*)"");}
        | I64                       {$$ = mkTypeNode(@$, TT_I64, (char*)"");}
        | U8                        {$$ = mkTypeNode(@$, TT_U8,  (char*)"");}
        | U16                       {$$ = mkTypeNode(@$, TT_U16, (char*)"");}
        | U32                       {$$ = mkTypeNode(@$, TT_U32, (char*)"");}
        | U64                       {$$ = mkTypeNode(@$, TT_U64, (char*)"");}
        | Isz                       {$$ = mkTypeNode(@$, TT_Isz, (char*)"");}
        | Usz                       {$$ = mkTypeNode(@$, TT_Usz, (char*)"");}
        | F16                       {$$ = mkTypeNode(@$, TT_F16, (char*)"");}
        | F32                       {$$ = mkTypeNode(@$, TT_F32, (char*)"");}
        | F64                       {$$ = mkTypeNode(@$, TT_F64, (char*)"");}
        | C8                        {$$ = mkTypeNode(@$, TT_C8,  (char*)"");}
        | C32                       {$$ = mkTypeNode(@$, TT_C32, (char*)"");}
        | Bool                      {$$ = mkTypeNode(@$, TT_Bool, (char*)"");}
        | Void                      {$$ = mkTypeNode(@$, TT_Void, (char*)"");}
        | usertype  %prec UserType  {$$ = mkTypeNode(@$, TT_Data, (char*)$1);}
        | '\'' ident                {$$ = mkTypeNode(@$, TT_TypeVar, (char*)$1);}
        ;

type: type '*'      %dprec 2             {$$ = mkTypeNode(@$, TT_Ptr,  (char*)"", new ArrayNode($1));}
    | type '[' ']'                       {$$ = mkTypeNode(@$, TT_Array,(char*)"", new ArrayNode($1));}
    | type '(' type_expr ')'             {$$ = mkTypeNode(@$, TT_Func, (char*)"", new ArrayNode($1));}  /* f-ptr w/ params*/
    | type '(' ')'                       {$$ = mkTypeNode(@$, TT_Func, (char*)"", new ArrayNode($1));}  /* f-ptr w/out params*/
    | '(' type_expr ')'       %prec MED  {$$ = $2;}
    | lit_type                           {$$ = $1;}
    ;

type_expr_: type_expr_ ',' type {$$ = setNext($1, $3);}
          | type_expr_ '|' type
          | type                {$$ = new ArrayNode($1);}
          ;

type_expr: type_expr_  {//Check if it is a tuple by checking the amount of types found
                        if(((ArrayNode*)$1)->exprs.size() > 1){
                            $$ = $1;
                        }else{ //tuple type
                            $$ = mkTypeNode(@$, TT_Tuple, (char*)"", $1);
                        }
                       }


modifier: Pub      {$$ = mkModNode(@$, Tok_Pub);} 
        | Pri      {$$ = mkModNode(@$, Tok_Pri);}
        | Pro      {$$ = mkModNode(@$, Tok_Pro);}
        | Raw      {$$ = mkModNode(@$, Tok_Raw);}
        | Const    {$$ = mkModNode(@$, Tok_Const);}
        | Noinit   {$$ = mkModNode(@$, Tok_Noinit);}
        | Pathogen {$$ = mkModNode(@$, Tok_Pathogen);}
        ;

modifier_list: modifier_list modifier {$$ = setNext($1, $2);}
             | modifier               {$$ = new ArrayNode($1);}
             ;


var_decl: maybe_mod_list Var ident '=' expr  {@$ = @3; $$ = mkVarDeclNode(@$, (char*)$3, $1,  0, $5);}
        ;

let_binding: Let modifier_list type_expr ident '=' expr {$$ = mkLetBindingNode(@$, (char*)$4, $2, $3, $6);}
           | Let modifier_list ident '=' expr           {$$ = mkLetBindingNode(@$, (char*)$3, $2, 0,  $5);}
           | Let type_expr ident '=' expr               {$$ = mkLetBindingNode(@$, (char*)$3, 0,  $2, $5);}
           | Let ident '=' expr                         {$$ = mkLetBindingNode(@$, (char*)$2, 0,  0,  $4);}
           ;


data_decl: modifier_list Data usertype type_decl_block         {$$ = mkDataDeclNode(@$, (char*)$3, $4);}
         | Data usertype type_decl_block                       {$$ = mkDataDeclNode(@$, (char*)$2, $3);}
         ;

type_decl: params
         | enum_decl
         ;

type_decl_list: type_decl_list Newline type_decl  {$$ = setNext($1, $3);}
              | type_decl                         {$$ = new ArrayNode($1);}
              ;

type_decl_block: Indent type_decl_list Unindent  {$$ = $2;}
               ;


/* Specifying an enum member's value */
val_init_list: val_init_list Newline usertype
             | val_init_list Newline usertype '=' expr
             | usertype '=' expr
             | usertype
             ;

enum_block: Indent val_init_list Unindent
          ;

enum_decl: modifier_list Enum usertype enum_block  {$$ = NULL;}
         | Enum usertype enum_block                {$$ = NULL;}
         | modifier_list Enum enum_block           {$$ = NULL;}
         | Enum enum_block                         {$$ = NULL;}
         ;


block: Indent expr Unindent  {$$ = $2;}
     ;



ident_list: ident_list var  {$$ = setNext($1, $2);}
          | var             {$$ = new ArrayNode($1);}
          ;


/* 
 * In case of multiple parameters declared with a single type, eg i32 a b c
 * The next parameter should be set to the first in the list, (the one returned by getRoot()),
 * but the variable returned must be the last in the last, in this case $4
 */


params_p: params_p ',' type_expr ident_list {$$ = addNamedValNode(@4, $1, $4, $3);}
        | type_expr ident_list              {$$ = addNamedValNode(@$, new ArrayNode(@$), $2, $1);}
        ;

                          /* varargs function .. (Range) followed by . */
params: params_p ',' Range '.' {$$ = addNamedValNode(@3, $1, new ArrayNode(mkVarNode(@$, (char*)"")), 0);}
      | params_p               {$$ = $1;}
      ;

maybe_mod_list: modifier_list  {$$ = $1;}
              | %empty         {$$ = 0;}
              ;

function: fn_def
        | fn_decl
        ;

fn_def: maybe_mod_list Fun ident ':' params Returns type_expr block  {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/$7,                                  /*params*/$5, /*body*/$8);}
      | maybe_mod_list Fun ident ':' Returns type_expr block         {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/$6,                                  /*params*/0,  /*body*/$7);}
      | maybe_mod_list Fun ident ':' params block                    {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/mkTypeNode(@$, TT_Void, (char*)""),  /*params*/$5, /*body*/$6);}
      | maybe_mod_list Fun ident ':' block                           {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/mkTypeNode(@$, TT_Void, (char*)""),  /*params*/0,  /*body*/$5);}
      ;

fn_decl: maybe_mod_list Fun ident ':' params Returns type_expr ';'       {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/$7, /*params*/$5, /*body*/0);}
       | maybe_mod_list Fun ident ':' Returns type_expr        ';'       {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/$6, /*params*/0,  /*body*/0);}
       | maybe_mod_list Fun ident ':' params                   ';'       {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/0,  /*params*/$5, /*body*/0);}
       | maybe_mod_list Fun ident ':'                          ';'       {$$ = mkFuncDeclNode(@$, /*fn_name*/(char*)$3, /*mods*/$1, /*ret_ty*/0,  /*params*/0,  /*body*/0);}
       ;



ret_expr: Return expr {$$ = mkRetNode(@$, $2);}
        ;


extension: Ext type_expr Indent fn_list Unindent {$$ = mkExtNode(@$, $2, $4);}
         ;


fn_list: fn_list function maybe_newline  {$$ = mkBinOpNode(@2, ';', $1, $2);} 
       | function maybe_newline          {$$ = $1;}
       ;


if_expr: If expr Then expr Else expr   %prec Else {$$ = mkIfNode(@$, $2, $4, $6);}
       | If expr Then expr             %prec LOW  {$$ = mkIfNode(@$, $2, $4,  0);}
       ;

while_loop: While expr Do expr  %prec LOW {$$ = mkWhileNode(@$, $2, $4);}
          ;

/*
do_while_loop: Do While expr Do expr {$$ = NULL;}
             ;

for_loop: For ident In expr expr {$$ = NULL;}
        ;
*/

var: ident  %prec Ident {$$ = mkVarNode(@$, (char*)$1);}
   ;


val: '(' expr ')'            {$$ = $2;}
   | tuple                   {$$ = $1;}
   | array                   {$$ = $1;}
   | unary_op                {$$ = $1;}
   | var                     {$$ = $1;}
   | intlit                  {$$ = $1;}
   | fltlit                  {$$ = $1;}
   | strlit                  {$$ = $1;}
   | True                    {$$ = mkBoolLitNode(@$, 1);}
   | False                   {$$ = mkBoolLitNode(@$, 0);}
   | let_binding             {$$ = $1;}
   | var_decl                {$$ = $1;}
   | if_expr                 {$$ = $1;}
   | while_loop              {$$ = $1;}
   | function                {$$ = $1;}
   | data_decl               {$$ = $1;}
   | extension               {$$ = $1;}
   | ret_expr                {$$ = $1;}
   | import_expr             {$$ = $1;}
   | block                   {$$ = $1;}
   ;

tuple: '(' expr_list ')' {$$ = mkTupleNode(@$, $2);}
     | '(' ')'           {$$ = mkTupleNode(@$, 0);}
     ;

array: '[' expr_list ']' {$$ = mkArrayNode(@$, $2);}
     | '[' ']'           {$$ = mkArrayNode(@$, 0);}
     ;


unary_op: '@' val                 {$$ = mkUnOpNode(@$, '@', $2);}
        | '&' val                 {$$ = mkUnOpNode(@$, '&', $2);}
        | '-' val                 {$$ = mkUnOpNode(@$, '-', $2);}
        | type_expr val           {$$ = mkTypeCastNode(@$, $1, $2);}
        ;


/* expr is used in expression blocks and can span multiple lines */
expr_list: expr_list ',' maybe_newline expr  %prec ',' {$$ = mkBinOpNode(@$, ';', $1, $4);}
         | expr                                %prec LOW {$$ = $1;}
         ;

expr: expr '+' maybe_newline expr                {$$ = mkBinOpNode(@$, '+', $1, $4);}
    | expr '-' maybe_newline expr                {$$ = mkBinOpNode(@$, '-', $1, $4);}
    | expr '*' maybe_newline expr                {$$ = mkBinOpNode(@$, '*', $1, $4);}
    | expr '/' maybe_newline expr                {$$ = mkBinOpNode(@$, '/', $1, $4);}
    | expr '%' maybe_newline expr                {$$ = mkBinOpNode(@$, '%', $1, $4);}
    | expr '<' maybe_newline expr                {$$ = mkBinOpNode(@$, '<', $1, $4);}
    | expr '>' maybe_newline expr                {$$ = mkBinOpNode(@$, '>', $1, $4);}
    | expr '.' maybe_newline var                 {$$ = mkBinOpNode(@$, '.', $1, $4);}
    | type_expr '.' maybe_newline var            {$$ = mkBinOpNode(@$, '.', $1, $4);}
    | expr ';' maybe_newline expr                {$$ = mkBinOpNode(@$, ';', $1, $4);}
    | expr Newline expr                          {$$ = mkBinOpNode(@$, ';', $1, $3);}
    | expr '[' expr ']'                          {$$ = mkBinOpNode(@$, '[', $1, $3);}
    | expr Eq maybe_newline expr                 {$$ = mkBinOpNode(@$, Tok_Eq, $1, $4);}
    | expr NotEq maybe_newline expr              {$$ = mkBinOpNode(@$, Tok_NotEq, $1, $4);}
    | expr GrtrEq maybe_newline expr             {$$ = mkBinOpNode(@$, Tok_GrtrEq, $1, $4);}
    | expr LesrEq maybe_newline expr             {$$ = mkBinOpNode(@$, Tok_LesrEq, $1, $4);}
    | expr Or maybe_newline expr                 {$$ = mkBinOpNode(@$, Tok_Or, $1, $4);}
    | expr And maybe_newline expr                {$$ = mkBinOpNode(@$, Tok_And, $1, $4);}
    | expr tuple                                 {$$ = mkBinOpNode(@$, '(', $1, $2);}
    | expr '=' maybe_newline expr                {$$ = mkVarAssignNode(@$, $1, $4);} /* All VarAssignNodes return void values */
    | expr AddEq maybe_newline expr              {$$ = mkVarAssignNode(@$, $1, mkBinOpNode(@$, '+', $1, $4), false);}
    | expr SubEq maybe_newline expr              {$$ = mkVarAssignNode(@$, $1, mkBinOpNode(@$, '-', $1, $4), false);}
    | expr MulEq maybe_newline expr              {$$ = mkVarAssignNode(@$, $1, mkBinOpNode(@$, '*', $1, $4), false);}
    | expr DivEq maybe_newline expr              {$$ = mkVarAssignNode(@$, $1, mkBinOpNode(@$, '/', $1, $4), false);}
    | expr Newline                               {$$ = $1;}
    | val                                        {$$ = $1;}
    ;

%%

/* location parser error */
void yy::parser::error(const location& loc, const string& msg){
    location l = loc;
    ante::error(msg.c_str(), l);
} 

/*
void yy::parser::error(const string& msg){
    ante::error(msg.c_str(), yylexer->fileName, yylexer->getRow(), yylexer->getCol());
}*/

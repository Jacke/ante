#ifndef PTREE_H
#define PTREE_H

#include "parser.h"

//defined in lexer.cpp
extern char* lextxt;

extern Node* rootNode;

#ifndef LOC_TY
#define LOC_TY yy::location
#endif

Node* mkIntLitNode(LOC_TY loc, char* s);
Node* mkFltLitNode(LOC_TY loc, char* s);
Node* mkStrLitNode(LOC_TY loc, char* s);
Node* mkBoolLitNode(LOC_TY loc, char b);
Node* mkArrayNode(LOC_TY loc, Node *expr);
Node* mkTupleNode(LOC_TY loc, Node *expr);
Node* mkModNode(LOC_TY loc, TokenType mod);
Node* mkTypeNode(LOC_TY loc, TypeTag type, char* typeName, Node *extTy = nullptr);
Node* mkTypeCastNode(LOC_TY loc, Node *l, Node *r);
Node* mkUnOpNode(LOC_TY loc, int op, Node *r);
Node* mkBinOpNode(LOC_TY loc, int op, Node* l, Node* r);

ArrayNode* setNext(Node *an, Node *nxt);
ArrayNode* addNamedValNode(yy::parser::location_type loc, Node* params, Node* varNodes, Node* tExpr);

Node* mkVarNode(LOC_TY loc, char* s);
Node* mkRetNode(LOC_TY loc, Node* expr);
Node* mkImportNode(LOC_TY loc, Node* expr);
Node* mkLetBindingNode(LOC_TY loc, char* s, Node* mods, Node* tExpr, Node* expr);
Node* mkVarDeclNode(LOC_TY loc, char* s, Node* mods, Node* tExpr, Node* expr);
Node* mkVarAssignNode(LOC_TY loc, Node* var, Node* expr, bool shouldFreeLval = true);
Node* mkExtNode(LOC_TY loc, Node* typeExpr, Node* methods);

Node* mkIfNode(LOC_TY loc, Node* con, Node* body, Node* els);
Node* mkWhileNode(LOC_TY loc, Node* con, Node* body);
Node* mkFuncDeclNode(LOC_TY loc, char* s, Node* mods, Node* tExpr, Node* p, Node* body);
Node* mkDataDeclNode(LOC_TY loc, char* s, Node* b);

#endif

#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory> //For unique_ptr
#include "lexer.h"
#include "tokens.h"
#include "compiler.h"
#include "location.hh"

enum ParseErr{
    PE_OK,
    PE_EXPECTED,
    PE_VAL_NOT_FOUND,
    PE_IDENT_NOT_FOUND,
    PE_INVALID_STMT,
};

using namespace llvm;
using namespace ante;

#ifndef LOC_TY
#  define LOC_TY yy::location
#endif

/* Base class for all nodes */
struct Node{
    LOC_TY loc;

    //print representation of node
    virtual void print(void) = 0;

    //compile node to a given module
    virtual TypedValue* compile(Compiler*) = 0;

    Node(LOC_TY& l) : loc(l){}
    virtual ~Node(){}
};

struct IntLitNode : public Node{
    string val;
    TypeTag type;
    TypedValue* compile(Compiler*);
    void print();
    IntLitNode(LOC_TY& loc, string s, TypeTag ty) : Node(loc), val(s), type(ty){}
    ~IntLitNode(){}
};

struct FltLitNode : public Node{
    string val;
    TypeTag type;
    TypedValue* compile(Compiler*);
    void print(void);
    FltLitNode(LOC_TY& loc, string s, TypeTag ty) : Node(loc), val(s), type(ty){}
    ~FltLitNode(){}
};

struct BoolLitNode : public Node{
    bool val;
    TypedValue* compile(Compiler*);
    void print(void);
    BoolLitNode(LOC_TY& loc, char b) : Node(loc), val(b){}
    ~BoolLitNode(){}
};

struct ArrayNode : public Node{
    vector<Node*> exprs;
    TypedValue* compile(Compiler*);
    void print(void);
    ArrayNode(LOC_TY& loc, vector<Node*>& e) : Node(loc), exprs(e){}
    ArrayNode(LOC_TY& loc) : Node(loc), exprs(){}
    ArrayNode(Node *n) : Node(n->loc), exprs(1, n){}
    ~ArrayNode(){}
};

struct TupleNode : public Node{
    vector<Node*> exprs;
    TypedValue* compile(Compiler*);
    vector<Value*> unpack(Compiler*);
    void print(void);
    TupleNode(LOC_TY& loc, vector<Node*>& e) : Node(loc), exprs(e){}
    ~TupleNode(){}
};

struct TypeCastNode : public Node{
    unique_ptr<TypeNode> typeExpr;
    unique_ptr<Node> rval;
    TypedValue* compile(Compiler*);
    void print(void);
    TypeCastNode(LOC_TY& loc, TypeNode *ty, Node *rv) : Node(loc), typeExpr(ty), rval(rv){}
    ~TypeCastNode(){}
};

struct UnOpNode : public Node{
    int op;
    unique_ptr<Node> rval;
    TypedValue* compile(Compiler*);
    void print(void);
    UnOpNode(LOC_TY& loc, int s, Node *rv) : Node(loc), op(s), rval(rv){}
    ~UnOpNode(){}
};

struct BinOpNode : public Node{
    int op;
    unique_ptr<Node> lval, rval;
    TypedValue* compile(Compiler*);
    void print(void);
    BinOpNode(LOC_TY& loc, int s, Node *lv, Node *rv) : Node(loc), op(s), lval(lv), rval(rv){}
    ~BinOpNode(){}
};

struct TypeNode : public Node{
    TypeTag type;
    string typeName; //used for usertypes
    unique_ptr<ArrayNode> extTys; //Used for pointers and non-single anonymous types.

    vector<Node*> getExts() const { return extTys->exprs; }
    TypedValue* compile(Compiler*);
    void print(void);
    TypeNode(LOC_TY& loc, TypeTag ty, string tName, ArrayNode* eTy) : Node(loc), type(ty), typeName(tName), extTys(eTy){}
    ~TypeNode(){}
};

struct ModNode : public Node{
    int mod;
    TypedValue* compile(Compiler*);
    void print(void);
    ModNode(LOC_TY& loc, int m) : Node(loc), mod(m){}
    ~ModNode(){}
};

struct RetNode : public Node{
    unique_ptr<Node> expr;
    TypedValue* compile(Compiler*);
    void print(void);
    RetNode(LOC_TY& loc, Node* e) : Node(loc), expr(e){}
    ~RetNode(){}
};

struct NamedValNode : public Node{
    string name;
    unique_ptr<Node> typeExpr;
    TypedValue* compile(Compiler*);
    void print(void);
    NamedValNode(LOC_TY& loc, string s, Node* t) : Node(loc), name(s), typeExpr(t){}
    ~NamedValNode(){}
};

struct VarNode : public Node{
    string name;
    TypedValue* compile(Compiler*);
    void print(void);
    VarNode(LOC_TY& loc, string s) : Node(loc), name(s){}
    ~VarNode(){}
};

struct StrLitNode : public Node{
    string val;
    TypedValue* compile(Compiler*);
    void print(void);
    StrLitNode(LOC_TY& loc, string s) : Node(loc), val(s){}
    ~StrLitNode(){}
};

struct LetBindingNode : public Node{
    string name;
    unique_ptr<Node> modifiers, typeExpr, expr;

    TypedValue* compile(Compiler*);
    void print(void);
    LetBindingNode(LOC_TY& loc, string s, Node *mods, Node* t, Node* exp) : Node(loc), name(s), modifiers(mods), typeExpr(t), expr(exp){}
    ~LetBindingNode(){}
};

struct VarDeclNode : public Node{
    string name;
    unique_ptr<Node> modifiers, typeExpr, expr;

    TypedValue* compile(Compiler*);
    void print(void);
    VarDeclNode(LOC_TY& loc, string s, Node *mods, Node* t, Node* exp) : Node(loc), name(s), modifiers(mods), typeExpr(t), expr(exp){}
    ~VarDeclNode(){}
};

struct VarAssignNode : public Node{
    Node* ref_expr;
    unique_ptr<Node> expr;
    bool freeLval;
    TypedValue* compile(Compiler*);
    void print(void);
    VarAssignNode(LOC_TY& loc, Node* v, Node* exp, bool b) : Node(loc), ref_expr(v), expr(exp), freeLval(b){}
    ~VarAssignNode(){ if(freeLval) delete ref_expr; }
};

struct ExtNode : public Node{
    unique_ptr<TypeNode> typeExpr;
    unique_ptr<Node> methods;
    TypedValue* compile(Compiler*);
    void print(void);
    ExtNode(LOC_TY& loc, TypeNode *t, Node *m) : Node(loc), typeExpr(t), methods(m){}
    ~ExtNode(){}
};

struct ImportNode : public Node{
    unique_ptr<Node> expr;
    TypedValue* compile(Compiler*);
    void print();
    ImportNode(LOC_TY& loc, Node* e) : Node(loc), expr(e){}
    ~ImportNode(){}
};

struct WhileNode : public Node{
    unique_ptr<Node> condition, body;
    TypedValue* compile(Compiler*);
    void print(void);
    WhileNode(LOC_TY& loc, Node *cond, Node *b) : Node(loc), condition(cond), body(b){}
    ~WhileNode(){}
};


//if node used in expressions
//requires elseN to be initialized, and
//typechecks thenN and elseN to be matching types.
struct IfNode : public Node{
    unique_ptr<Node> condition, thenN, elseN;
    TypedValue* compile(Compiler*);
    void print(void);
    IfNode(LOC_TY& loc, Node* c, Node* then, Node* els) : Node(loc), condition(c), thenN(then), elseN(els){}
    ~IfNode(){}
};

struct FuncDeclNode : public Node{
    string name;
    unique_ptr<Node> modifiers, type, body;
    unique_ptr<ArrayNode> params;
    bool varargs;

    TypedValue* compile(Compiler*);
    void print(void);
    FuncDeclNode(LOC_TY& loc, string s, Node *mods, Node *t, ArrayNode* p, Node* b, bool va=false) : Node(loc), name(s), modifiers(mods), type(t), body(b), params(p), varargs(va){}
    ~FuncDeclNode(){}
};

struct DataDeclNode : public Node{
    string name;
    size_t fields;
    unique_ptr<ArrayNode> body;

    TypedValue* compile(Compiler*);
    void print(void);
    DataDeclNode(LOC_TY& loc, string s, ArrayNode* b, size_t f) : Node(loc), name(s), fields(f), body(b){}
    ~DataDeclNode(){}
};


namespace ante{
    namespace parser{
        Node* getRootNode();
        void parseErr(ParseErr e, string s, bool showTok);
    }
}

void printErrLine(const char* fileName, unsigned int row, unsigned int col);

#endif

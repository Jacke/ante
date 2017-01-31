#ifndef COMPILER_H
#define COMPILER_H

#include <climits> //required by llvm when using clang
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <memory>
#include <map>
#include <list>
#include "parser.h"
#include "args.h"
#include "lazystr.h"

using namespace llvm;
using namespace std;


Node* getNthNode(Node *node, size_t n);
TypeNode* deepCopyTypeNode(const TypeNode *n);
string typeNodeToStr(const TypeNode *t);
lazy_str typeNodeToColoredStr(const TypeNode *t);
lazy_str typeNodeToColoredStr(const unique_ptr<TypeNode>& tn);

/*
 *  Used for storage of additional information, such as signedness,
 *  not represented by llvm::Type
 */
struct TypedValue {
    Value *val;
    unique_ptr<TypeNode> type;

    TypedValue(Value *v, TypeNode *ty) : val(v), type(ty){}
    TypedValue(Value *v, unique_ptr<TypeNode> &ty) : val(v), type(deepCopyTypeNode(ty.get())){}
    
    Type* getType() const{ return val->getType(); }
    bool hasModifier(int m) const{ return type->hasModifier(m); }
    void dump() const;
};

bool isPrimitiveTypeTag(TypeTag ty);
TypeNode* mkAnonTypeNode(TypeTag);
TypeNode* mkTypeNodeWithExt(TypeTag tt, TypeNode *ext);
TypeNode* mkDataTypeNode(string tyname);



//result from a typecheck function
//gives not only a success/failure flag but also the specific
//typevars binded and what they were binded to.
struct TypeCheckResult {
    enum Result { Failure, Success, SuccessWithTypeVars };

    Result res;
    vector<pair<string, unique_ptr<TypeNode>>> bindings;


    TypeCheckResult* setRes(bool b);
    TypeCheckResult* setRes(Result r);
    TypeCheckResult* setSuccess(){ if(res != SuccessWithTypeVars) res = Success; return this; }
    TypeCheckResult* setSuccessWithTypeVars(){ res = SuccessWithTypeVars; return this; }
    TypeCheckResult* setFailure(){ res = Failure; return this; }

    bool operator!(){return res == Failure;}
    TypeNode* getBindingFor(const string &s);
    TypeCheckResult(Result r) : res(r){}
    TypeCheckResult(bool r) : res((Result)r){}
};

TypeCheckResult typeEqBase(const TypeNode *l, const TypeNode *r, TypeCheckResult *tcr, const Compiler *c = 0);



//declare ante::Module for FuncDecl
namespace ante { struct Module; }

/*
 * FuncDeclNode and int pair to retain a function's
 * scope after it is imported and lazily compiled later
 * in a seperate scope.
 */
struct FuncDecl {
    FuncDeclNode *fdn;
    unsigned int scope;
    TypedValue *tv;
    shared_ptr<ante::Module> module;
    FuncDecl(FuncDeclNode *fn, unsigned int s, shared_ptr<ante::Module> mod, TypedValue *f=0) : fdn(fn), scope(s), tv(f), module(mod){}
    ~FuncDecl(){ if(fdn){delete fdn;} delete tv; }
};

struct MethodVal : public TypedValue {
    TypedValue *obj;

    MethodVal(TypedValue *o, TypedValue *f) : TypedValue(f->val, (f->type->type = TT_Method, f->type.get())), obj(o) {}
};

struct UnionTag {
    string name;
    unique_ptr<TypeNode> tyn;
    unsigned short tag;
    
    UnionTag(string &n, TypeNode *ty, unsigned short t) : name(n), tyn(ty), tag(t){}
};

struct Trait {
    string name;
    vector<shared_ptr<FuncDecl>> funcs;
};

struct DataType {
    vector<string> fields;
    vector<unique_ptr<UnionTag>> tags;
    vector<shared_ptr<Trait>> traitImpls;
    unique_ptr<TypeNode> tyn;

    DataType(const vector<string> &f, TypeNode *ty) : fields(f), tyn(ty){}
    ~DataType(){}

    int getFieldIndex(string &field) const {
        for(unsigned int i = 0; i < fields.size(); i++)
            if(field == fields[i])
                return i;
        return -1;
    }

    bool isUnionTag() const {
        return fields[0][0] >= 'A' && fields[0][0] <= 'Z';
    }

    string getParentUnionName() const {
        return fields[0];
    }

    unsigned short getTagVal(string &name){
        for(auto& tag : tags){
            if(tag->name == name){
                return tag->tag;
            }
        }
        return 0;
    }
};

struct Variable {
    string name;
    unique_ptr<TypedValue> tval;
    unsigned int scope;
    bool noFree, autoDeref;

    Value* getVal() const{
        return tval->val;
    }

    TypeTag getType() const{
        return tval->type->type;
    }

    bool isFreeable() const{
        return tval->type? tval->type->type == TT_Ptr && !noFree : false;
    }

    Variable(string n, TypedValue *tv, unsigned int s, bool nofr=true, bool autoDr=false) : name(n), tval(tv), scope(s), noFree(nofr), autoDeref(autoDr){}
};

/* structure that holds a c++ function */
/* Differs from std::function in that it is
 * not template differentiated based on type
 * of the function */
struct CtFunc {
    void *fn;
    vector<unique_ptr<TypeNode>> params;
    unique_ptr<TypeNode> retty;

    size_t numParams() const { return params.size(); }
    bool typeCheck(vector<TypeNode*> &args);
    bool typeCheck(vector<TypedValue*> &args);
    CtFunc(void* fn);
    CtFunc(void* fn, TypeNode *retTy);
    CtFunc(void* fn, TypeNode *retTy, vector<unique_ptr<TypeNode>> params);
    void* operator()();
    void* operator()(TypedValue *tv);
};


//forward-declare location for compErr and ante::err
namespace yy{ class location; }

namespace ante{

    struct Compiler;

    struct Module {
        string name;
       
        //declared functions
        map<string, list<shared_ptr<FuncDecl>>> fnDecls;

        //Map of declared usertypes
        map<string, shared_ptr<DataType>> userTypes;

        //Map of all declared traits; not including their implementations for a given type
        //Each DataType is reponsible for holding its own trait implementations
        map<string, shared_ptr<Trait>> traits;

        void import(Compiler *c, shared_ptr<ante::Module> m);
    };

    struct Compiler {
        shared_ptr<LLVMContext> ctxt;
        unique_ptr<ExecutionEngine> jit;
        unique_ptr<legacy::FunctionPassManager> passManager;
        unique_ptr<llvm::Module> module;
        unique_ptr<Node> ast;
        IRBuilder<> builder;

        //functions and type definitions of current module
        shared_ptr<ante::Module> compUnit;

        //all functions and type definitions visible to current module
        shared_ptr<ante::Module> mergedCompUnits;

        //all imported modules
        vector<shared_ptr<ante::Module>> imports;
        
        //every single compiled module, even ones invisible to the current
        //compilation_unit.  Prevents recompilation of modules
        map<string, shared_ptr<ante::Module>> allCompiledModules;
        
        //Stack of maps of variables mapped to their identifier.
        //Maps are seperated according to their scope.
        vector<unique_ptr<std::map<string, unique_ptr<Variable>>>> varTable;


        bool errFlag, compiled, isLib;
        string fileName, outFile, funcPrefix;
        unsigned int scope;

        Compiler(const char *fileName, bool lib=false);
        Compiler(Node *root, string modName, string &fName, bool lib=false);
        ~Compiler();

        void compile();
        void compileNative();
        int  compileObj(string &outName);
        void compilePrelude();
        Function* createMainFn();
        void eval();
        void emitIR();
        
        void enterNewScope();
        void exitScope();
        
        void scanAllDecls();
        void processArgs(CompilerArgs *args);
        
        //binop functions
        TypedValue* compAdd(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compSub(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compMul(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compDiv(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compRem(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compExtract(TypedValue *l, TypedValue *r, BinOpNode *op);
        TypedValue* compInsert(BinOpNode *insertOp, Node *assignExpr);
        TypedValue* compMemberAccess(Node *ln, VarNode *field, BinOpNode *binop);
        TypedValue* compLogicalOr(Node *l, Node *r, BinOpNode *op);
        TypedValue* compLogicalAnd(Node *l, Node *r, BinOpNode *op);
       
        //TypedValue* compErr(const string msg, const yy::location& loc);
        TypedValue* compErr(ante::lazy_printer msg, const yy::location& loc);

        void jitFunction(Function *fnName);
        void importFile(const char *name);
        void updateFn(TypedValue *f, string &name, string &mangledName);
        TypedValue* getFunction(string& name, string& mangledName);
        list<shared_ptr<FuncDecl>>& getFunctionList(string& name);
        TypedValue* getMangledFunction(string nonMangledName, TypeNode *params);
        TypedValue* getCastFn(TypeNode *from_ty, TypeNode *to_ty);
        
        TypedValue* compLetBindingFn(FuncDecl *fdn, vector<Type*> &paramTys);
        TypedValue* compFn(FuncDecl *fn);
        void registerFunction(FuncDeclNode *func);

        unsigned int getScope() const;
        Variable* lookup(string var) const;
        void stoVar(string var, Variable *val);
        DataType* lookupType(string tyname) const;
        Trait* lookupTrait(string tyname) const;
        void stoType(DataType *ty, string &typeName);

        Type* typeNodeToLlvmType(const TypeNode *tyNode);
        TypeCheckResult typeEq(const TypeNode *l, const TypeNode *r) const;
    
        TypedValue* opImplementedForTypes(int op, TypeNode *l, TypeNode *r);
        TypedValue* implicitlyWidenNum(TypedValue *num, TypeTag castTy);
        void handleImplicitConversion(TypedValue **lhs, TypedValue **rhs);
        void implicitlyCastIntToInt(TypedValue **lhs, TypedValue **rhs);
        void implicitlyCastFltToFlt(TypedValue **lhs, TypedValue **rhs);
        void implicitlyCastIntToFlt(TypedValue **tval, Type *ty);
        
        int compileIRtoObj(llvm::Module *mod, string outFile);

        TypedValue* getVoidLiteral();
        static int linkObj(string inFiles, string outFile);
    };
}

size_t getTupleSize(Node *tup);
string mangle(string &base, TypeNode *paramTys);

//conversions
Type* typeTagToLlvmType(TypeTag tagTy, LLVMContext &c, string typeName = "");
TypeTag llvmTypeToTypeTag(Type *t);
string llvmTypeToStr(Type *ty);
string typeTagToStr(TypeTag ty);
bool llvmTypeEq(Type *l, Type *r);

char getBitWidthOfTypeTag(const TypeTag tagTy);
bool isNumericTypeTag(const TypeTag ty);
bool isIntTypeTag(const TypeTag ty);
bool isFPTypeTag(const TypeTag tt);
bool isUnsignedTypeTag(const TypeTag tagTy);

string removeFileExt(string file);
#endif

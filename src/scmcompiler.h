#pragma once

#include <cstdio>
namespace Scheme {

class MatchObj {
public:
  MatchObj(VM* v, ValueT* m):vm(v), matchvt(m) {
    setarray(matchvt, &matches);
  }
  void addmatch(ValueT* sym, ValueT* expr);
  PairPtr getsymmatch(SymPtr sym);
  void merge(MatchObj* sub, int idx);
  int matchcount();
  void reuse();

  VM* vm;
  ValueT* matchvt;
  ArrayObj matches;
};

struct PatnVarDepth {
  SymPtr sym;
  int depth;

  void visit(VM* vm) {
    Check(sym);
  }
  PatnVarDepth():sym(NULL), depth(-1) {}
};

struct MatchState {
  SCompiler* lstate;
  ValueT* literals;
  ValueT* expr;
  IntArray* idxarr;
  MatchObj* matches;
};

class PatnTmpl {
public:
  void init(VM* vm, ValueT* literals, ValueT* );
  void initpatn(VM* vm, ValueT* literals, ValueT*);
  void initpatn(VM* vm, ValueT* literals, ValueT* , int);
  void inittmpl(VM* vm, ValueT* literals, ValueT*, ValueT*, int);
  void checktmplsym(VM* vm, ValueT* literals, ValueT* sym, ValueT*, int depth);

  bool trymatchrepeat2(ArrayObj* arr, int idx, ValueT* tomatch, int depth, MatchState* state);
  bool trymatchrepeat1(ValueT* expr, ValueT* tomatch, int depth, MatchState* state);
  bool trymatchpair(ValueT* expr, ValueT* ptn1, ValueT* ptn2, int depth, MatchState* state);
  bool trymatch(ValueT* expr, ValueT* ptn, int depth, MatchState* state);

  void expandarray(ValueT* out, ValueT* tpl, int i, int depth, MatchState* state);
  void expandpair(ValueT* out, ValueT* atpl, ValueT* dtpl, int depth, MatchState* state);
  void expand(ValueT* out, ValueT* tpl, int depth, MatchState* state);

  bool isliteral(ValueT* literals, SymPtr sym) {
    PAIR_FOR(p, literals)
      if (sym == symref(Scar(p)))
        return true;
    return false;
  }
  void addvar(VM* vm, SymPtr sym, int d);
  PatnVarDepth* ispatnvar(SymPtr sym);
  void visit(VM* vm) {
    VEC_FOR(i, &allvars)
      allvars.getptr(i)->visit(vm);
    Check(patn);
    Check(tmpl);
  }
  void finz(VM* vm) {
    vec_finz(PatnVarDepth, vm, &allvars);
  }
  void shrink(VM* vm) {
    vec_shrink(PatnVarDepth, vm, &allvars);
  }
  VecT<PatnVarDepth> allvars;
  ValueT patn;
  ValueT tmpl;
};

class SyntaxRules : public RefObject {
public:
  PatnTmpl* expand(SCompiler* state, ValueT* out, ValueT* expr);
  PatnTmpl* newsrule(VM* vm);
  void addlits(VM* vm, SymPtr lit, ValueT* expr);
  virtual void visit(VM* vm) {
    Check(literals);
    VEC_FOR(i, &rules)
      rules.get(i).visit(vm);
  }
  virtual void finz(VM* vm);
  void shrink(VM* vm);

  GetSize(SyntaxRules)

  ValueT literals;
  VecT<PatnTmpl> rules;
};

struct SyntaxObj : public RefObject {
  SyntaxObj(SymPtr n, SyntaxRules* p):name(n), proc(p) {}

  GetSize(SyntaxObj)
  Visit2(name, proc)

  SymPtr name;
  SyntaxRules* proc;
};

class SCompiler {
public:
  SCompiler(VM* v, LambdaPtr lam, SCompiler* enclosing, ValueT* n2h):
    vm(v), lambda(lam), enclose(enclosing), name2hy(n2h),
    prevline(0) {}
  ~SCompiler();
public:
  void compilelink(ValueT* link, int line);
  void compile(int target, ValueT* val, ValueT* link, bool defok);
  void compilesym(int target, ValueT *sym, ValueT *link);
  void compilerefsym(int* local, int* refvar, SymPtr sym);
  void compilepair(int target, ValueT* pair, ValueT* link, bool defok);
  void compileseqpre(ValueT* expr);
  void compileseqpre0(ValueT* vt);
  void compileseq(int target, ValueT* pair, ValueT* link, bool defok);
  void compilelambda(int target, ValueT* pair, ValueT* link);
  void compilelambda0(int target, int line, ValueT* param, ValueT* body);
  void compilesyntaxrules(ValueT* pair, ValueT* out);
  void compiledefsyntax(ValueT*);
  void compileif(int target, ValueT* pair, ValueT* link);
  void compiledefpre(ValueT* val);
  void compiledef(int target, ValueT* pair, ValueT* link);
  void compiledefsym(int target, ValueT* symvt, ValueT* expr, ValueT* link);
  int  compiledefsym0(int target, ValueT* symvt);
  void compiledeflambda(int target, ValueT* vt, ValueT* body, ValueT* link);
  void compileset(int target, ValueT* pair, ValueT* link);
  void compileapp(int target, ValueT* val, ValueT* link);

  void compileqquote(int target, ValueT* val, int depth);
  void compileqquotecons(int target, ValueT* a, ValueT* d, int depth);
  void compileqquotearray(int target, ValueT* expr, int depth);

  void compileappargs(int target, ValueT* args);
public:
  void putcode(Instruction i,  int line);
  SymPtr getsym(ValueT* symvt);

  bool finddef(SymPtr sym);
  void adddef(SymPtr sym);

  void sym2hygiene(ValueT* name, ValueT* out);

  SyntaxPtr getsyntax0(SymPtr symtype);
  SyntaxPtr getsyntax(ValueT* type, ValueT* out);

public:
  int prevline;
  SCompiler* enclose;
  ValueT defs;
  ValueT* name2hy;
  LambdaPtr lambda;
  VM* vm;
};

}

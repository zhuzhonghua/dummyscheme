#pragma once

#include "util.h"
#include "refcount.h"

namespace Dummy {

class Value;
typedef RefVarPtr<Value>  VarValue;
typedef RefMemberPtr<Value>  MemberValue;

typedef VarValue (*PrimProc0)();
typedef VarValue (*PrimProc1)(VarValue arg);
typedef VarValue (*PrimProc2)(VarValue arg1, VarValue arg2);
typedef VarValue (*PrimProc3)(VarValue arg1, VarValue arg2, VarValue arg3);
typedef VarValue (*PrimProc4)(VarValue arg1, VarValue arg2, VarValue arg3, VarValue arg4);

class PrimProc{
public:
  PrimProc(int num, PrimProc0 p): argNum(num) { proc.proc0 = p; }
  PrimProc(int num, PrimProc1 p): argNum(num) { proc.proc1 = p; }
  PrimProc(int num, PrimProc2 p): argNum(num) { proc.proc2 = p; }
  PrimProc(int num, PrimProc3 p): argNum(num) { proc.proc3 = p; }

public:
  int argNum;
  union {
    PrimProc0 proc0;
    PrimProc1 proc1;
    PrimProc2 proc2;
    PrimProc3 proc3;
    PrimProc4 proc4;
  }proc;
};

struct RegPrimProc{
  const char* name;
  PrimProc proc;
};

class Scheme {
protected:
  typedef std::map<String, VarValue> SymbolMap;
  typedef SymbolMap::iterator SymbolMapItr;
  static SymbolMap constSyms;

  typedef std::map<VarValue, PrimProc> ProcMap;
  typedef ProcMap::iterator ProcMapItr;
  static ProcMap primProcs;

  static void initPrimProc();
  static void regPrimProcs(RegPrimProc* procs, int num);
  static void regPrimProc(const String& name, PrimProc proc);
  static void initIntern();
public:
  static bool isPrimProc(VarValue exp);
  static void init();
public:
  static VarValue Void;
  static VarValue Nil;
  static VarValue Null;
  static VarValue True;
  static VarValue False;

  static VarValue Quote;
  static VarValue UnQuote;
  static VarValue UnQuoteSplicing;
  static VarValue QuasiQuote;

  static VarValue SetMark;
  static VarValue Define;
  static VarValue Lambda;
  static VarValue If;
  static VarValue Procedure;
  static VarValue Begin;
  static VarValue Cond;
  static VarValue Else;

  static VarValue notp(VarValue exp);
  static VarValue truep(VarValue exp);
  static VarValue falsep(VarValue exp);
  static VarValue eqp(VarValue e1, VarValue e2);
  static VarValue booleanp(VarValue exp);
  static VarValue pairp(VarValue val);
  static VarValue stringp(VarValue val);
  static VarValue symbolp(VarValue val);
  static VarValue numberp(VarValue val);
  static VarValue quotedp(VarValue val);
  static VarValue nullp(VarValue exp);

  static VarValue plus(VarValue arg);
  static VarValue cons(VarValue arg1, VarValue arg2);
  static VarValue list(VarValue arg);
  static VarValue car(VarValue val);
  static VarValue cdr(VarValue val);
  static VarValue set_car(VarValue val, VarValue a);
  static VarValue set_cdr(VarValue val, VarValue d);
  static VarValue cadr(VarValue val);
  static VarValue cddr(VarValue val);
  static VarValue caadr(VarValue val);
  static VarValue caddr(VarValue val);
  static VarValue cdadr(VarValue val);
  static VarValue cdddr(VarValue val);
  static VarValue cadddr(VarValue val);

public:
  static VarValue intern(const String& symbol);
  static VarValue intern(const String& symbol, VarValue value);
  static VarValue list2(VarValue a, VarValue b);
  static VarValue list3(VarValue a, VarValue b, VarValue c);
  static VarValue list4(VarValue a, VarValue b, VarValue c, VarValue d);
  static VarValue listn(int num, ...);

  static VarValue quote(VarValue exp) { return list2(Quote, exp); }
  static VarValue unquote(VarValue exp) { return list2(UnQuote, exp); }
  static VarValue quasiquote(VarValue exp) { return list2(QuasiQuote, exp); }
  static VarValue unquotesplicing(VarValue exp) { return list2(UnQuoteSplicing, exp); }

  static VarValue tagged_list_p(VarValue val, VarValue tag);
  static VarValue text_of_quotation(VarValue val);

  static VarValue self_evaluating_p(VarValue val);


  static VarValue assignmentp(VarValue val);
  static VarValue assignment_variable(VarValue val);
  static VarValue assignment_value(VarValue val);

  static VarValue definep(VarValue val);
  static VarValue define_variable(VarValue val);
  static VarValue define_value(VarValue val);

  static VarValue ifp(VarValue exp);
  static VarValue if_predicate(VarValue exp);
  static VarValue if_consequent(VarValue exp);
  static VarValue if_alternative(VarValue exp);
  static VarValue make_if(VarValue predicate, VarValue consequent, VarValue alternative);

  static VarValue make_lambda(VarValue parameters, VarValue body);
  static VarValue lambdap(VarValue exp);
  static VarValue lambda_parameters(VarValue exp);
  static VarValue lambda_body(VarValue exp);

  static VarValue analyze_sequence(VarValue exps);

  static VarValue make_procedure(VarValue parameters, VarValue body, VarValue env);
  static VarValue compound_procedure_p(VarValue exp);
  static VarValue procedure_parameters(VarValue exp);
  static VarValue procedure_body(VarValue exp);
  static VarValue procedure_env(VarValue exp);

  static VarValue last_exp_p(VarValue seq);
  static VarValue first_exp(VarValue seq);
  static VarValue rest_exp(VarValue seq);

  static VarValue beginp(VarValue exp);
  static VarValue begin_actions(VarValue exp);
  static VarValue make_begin(VarValue seq);

  static VarValue condp(VarValue exp);
  static VarValue cond_clauses(VarValue exp);
  static VarValue cond_else_clause_p(VarValue clause);
  static VarValue cond_predicate(VarValue clause);
  static VarValue cond_actions(VarValue clause);
  static VarValue cond_to_if(VarValue exp);

  static VarValue expand_clauses(VarValue clauses);

  static VarValue sequence_to_exp(VarValue seq);

  static VarValue operatr(VarValue exp);
  static VarValue operands(VarValue exp);
  static VarValue no_operands_p(VarValue ops);
  static VarValue first_operand(VarValue ops);
  static VarValue rest_operands(VarValue ops);

  static VarValue analyze_application(VarValue exp);
  static VarValue applicationp(VarValue exp);
  static VarValue execute_application(VarValue proc, VarValue args);

  static VarValue extend_env(VarValue parameters, VarValue args, VarValue outer);
public:
  static VarValue analyze(VarValue exp);
public:
  static VarValue reverse(VarValue );
};

};

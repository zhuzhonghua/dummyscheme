#pragma once

namespace Scheme {

#define SIZE_OP		8
#define SIZE_2H   12
#define SIZE_3T   8

#define MASK(n) (~((~((Instruction)0))<<(n)))

#define GET_OP(i) ((i)&MASK(SIZE_OP))
#define CREATE_OP(op) (op)
#define CREATE_OPA(op,a) (((a)<<SIZE_OP) | (op))
#define GET_OPA(i,a) (a)=((i)>>SIZE_OP)

#define CREATE_OPAB(op,a,b) (((((b)<<SIZE_OP))<<SIZE_2H) |  \
                             ((a)<<SIZE_OP)              |  \
                             (op))
#define GET_OPAB(i,a,b)                         \
  (a)=((i)>>SIZE_OP)&MASK(SIZE_2H),             \
  (b)=((((i)>>SIZE_OP))>>SIZE_2H)

#define CREATE_OPABC(op,a,b,c) ((((((c)<<SIZE_OP))<<SIZE_3T)<<SIZE_3T) | \
                                ((((b)<<SIZE_OP))<<SIZE_3T)            | \
                                ((a)<<SIZE_OP)                         | \
                                (op))
#define GET_OPABC(i,a,b,c)                      \
  (a)=(((i)>>SIZE_OP)&MASK(SIZE_3T)),           \
  (b)=((((i)>>SIZE_OP)>>SIZE_3T)&MASK(SIZE_3T)),\
  (c)=((((i)>>SIZE_OP)>>SIZE_3T)>>SIZE_3T)

enum OPCode {
  OP_CONS,
  OP_CONSEXT,
  OP_LIST,
  OP_LISTK,
  OP_LIST2VEC,
  OP_APPEND,
  OP_APPEND0,
  OP_APPENDEXT,
  OP_CALLAPP,
  OP_TAILCALLAPP,
  OP_LAMBDA,
  OP_IFFALSEJUMP,
  OP_SETLOCAL,
  OP_SETOVAR,
  OP_SETGLOBAL,
  OP_DEFGLOBAL,
  OP_VARREFLOCAL,
  OP_VARREFGLOBAL,
  OP_VARREFOVAR,
  OP_ASSIGN,
  OP_JUMPLABEL,
  OP_RETURN,
};

#define casecode(OP) case OP

#define code_cons2(A, B, C) CREATE_OPABC(OP_CONSEXT, A, B, C)
#define getcode_cons2(i, A, B, C) GET_OPABC(i, A, B, C)

#define code_cons(A) CREATE_OPA(OP_CONS, A)
#define getcode_cons(i, A) GET_OPA(i, A)

#define code_append2(A, B, C) CREATE_OPABC(OP_APPENDEXT, A, B, C)
#define getcode_append2(i, A, B, C) GET_OPABC(i, A, B, C)

#define code_append0(A) CREATE_OPA(OP_APPEND0, A)
#define getcode_append0(i, A) GET_OPA(i, A)

#define code_append(A) CREATE_OPA(OP_APPEND, A)
#define getcode_append(i, A) GET_OPA(i, A)

#define code_list(A) CREATE_OPA(OP_LIST, A)
#define getcode_list(i, A) GET_OPA(i, A)

#define code_listk(A, B) CREATE_OPAB(OP_LISTK, A, B)
#define getcode_listk(i, A, B) GET_OPAB(i, A, B)

#define code_list2vec(A) CREATE_OPA(OP_LIST2VEC, A)
#define getcode_list2vec(i, A) GET_OPA(i, A)

#define code_ret() CREATE_OP(OP_RETURN)

#define code_jmplabel(offset) CREATE_OPA(OP_JUMPLABEL, offset)
#define getcode_jmplabel(i, offset) GET_OPA(i, offset)

#define code_assign(target, from) CREATE_OPAB(OP_ASSIGN, target, from)
#define getcode_assign(i, target, from) GET_OPAB(i, target, from)

#define code_varrefovar(target, from) CREATE_OPAB(OP_VARREFOVAR, target, from)
#define getcode_varrefovar(i, target, from) GET_OPAB(i, target, from)

#define code_varreflocal(target, from) CREATE_OPAB(OP_VARREFLOCAL, target, from)
#define getcode_varreflocal(i, target, from) GET_OPAB(i, target, from)

#define code_varrefglobal(target, from) CREATE_OPAB(OP_VARREFGLOBAL, target, from)
#define getcode_varrefglobal(i, target, from) GET_OPAB(i, target, from)

#define code_defglobal(A, B) CREATE_OPAB(OP_DEFGLOBAL, A, B)
#define getcode_defglobal(i, A, B) GET_OPAB(i, A, B)

#define code_setlocal(A, B) CREATE_OPAB(OP_SETLOCAL, A, B)
#define getcode_setlocal(i, A, B) GET_OPAB(i, A, B)

#define code_setovar(A, B) CREATE_OPAB(OP_SETOVAR, A, B)
#define getcode_setovar(i, A, B) GET_OPAB(i, A, B)

#define code_setglobal(A, B) CREATE_OPAB(OP_SETGLOBAL, A, B)
#define getcode_setglobal(i, A, B) GET_OPAB(i, A, B)

#define code_iffalsejmp(A, B) CREATE_OPAB(OP_IFFALSEJUMP, A, B)
#define getcode_iffalsejmp(i, A, B) GET_OPAB(i, A, B)

#define code_lambda(A, B) CREATE_OPAB(OP_LAMBDA, A, B)
#define getcode_lambda(i, A, B) GET_OPAB(i, A, B)

#define code_callapp(k, len) CREATE_OPAB(OP_CALLAPP, k, len)
#define getcode_callapp(i, k, len) GET_OPAB(i, k, len)

#define code_tailcallapp(k, len) CREATE_OPAB(OP_TAILCALLAPP, k, len)
#define getcode_tailcallapp(i, k, len) GET_OPAB(i, k, len)
};

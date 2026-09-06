#pragma once

#include "vm.h"
namespace Scheme {

class SCMMath {
public:
  static void init(VM* vm);

  static bool str2num(VM* vm, const char* s, int len, ValueT* out);

  static bool numequalp(ValueT* p1, ValueT* p2);
  static bool numbiggerp(ValueT* p1, ValueT* p2);
  static bool numlessp(ValueT* p1, ValueT* p2);
  static bool exactp(ValueT* p);
  static bool isnumzero(VM* vm, ValueT* a);
  static bool toreal(ValueT* z, double* v);
  static void complexrect2polarmag(double x1,double x2,double *x3);
  static void complexrect2polarangle(double x1,double x2,double *x3);
  static void complexpolar2rectreal(double x1,double x2,double *x3);
  static void complexpolar2rectimag(double x1,double x2,double *x3);
  static void float2ratio(scm_float,scm_float*,scm_int *nu,scm_int *de, int maxiter, scm_float tol);

  static bool isFloatEqual(scm_float a, scm_float b);
  static scm_int gcd(scm_int a, scm_int b);
  static NumBigObj* gcd(VM* vm, NumBigObj* a, NumBigObj* b);
  static scm_int lcm(scm_int a, scm_int b);
  static NumBigObj* lcm(VM* vm, NumBigObj* a, NumBigObj* b);

  static void numbigdiv(VM* vm, NumBigObj* a, NumBigObj* b, NumBigObj**, NumBigObj** );
  static NumBigDivRes numbigdivabs(VM* vm, NumBigObj* a, NumBigObj* b);
};
};

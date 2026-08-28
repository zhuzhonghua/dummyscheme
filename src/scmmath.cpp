#include "vm.h"
#include "scmmath.h"

namespace Scheme {

/*
** ==================================================================
** Math
** ==================================================================
*/

// 0=int,1=real,2=ratio,3=complex
enum NType {
  NINT,
  NREAL,
  NRATIO,
  NCOMPLEX,
};

struct NumRatio {
  scm_int nu;
  scm_int de;
};

union UCoNum {
  scm_int inum;
  scm_float real;
  NumRatio ratio;
};

struct VCoNum {
  byte type;
  UCoNum num;

  void setratio(scm_int nu, scm_int de) {
    type = NRATIO;
    num.ratio.nu = nu; num.ratio.de = de;
  }
  void setreal(scm_float real) {
    type = NREAL;
    num.real = real;
  }
  void setint(scm_int real) {
    type = NINT;
    num.inum = real;
  }
};

struct ReadNumState {
  bool exactp;
  bool negativep;
  int radix;
  int c;
  int zn;
  const char* zp;
  VCoNum n;
  NumBigObj* bigval;

  bool isend() { return zn <= 0; }
  bool next() { c = zn-- > 0 ? *zp++ : -1; return c >= 0; }
};

static bool scm_readnum(VM* vm, ReadNumState* state, ValueT* out);

static double big2double(NumBigObj* b)
{
  static const double RADIX_2_63 = 1ULL << (INT_BITS-1);
  static const double RADIX_2_64 = RADIX_2_63 * 2;
  double result = 0;
  for (int i = b->len-1; i>=0; i--)
  {
    result = result * RADIX_2_64;
    result = result + (double)b->data[i];
  }
  return b->sign < 0 ? -result : result;
}


bool SCMMath::str2num(VM* vm, const char* s, int len, ValueT* out)
{
  ReadNumState state;
  state.exactp = true;
  state.negativep = false;
  state.radix = 10;
  state.zp = s;
  state.zn = len;
  state.bigval = NULL;
  state.next();
  return scm_readnum(vm, &state, out);
}

bool SCMMath::isnumzero(VM* vm, ValueT* a)
{
  switch(vttype(a)) {
  case VT_NUM_INTEGER:
    return 0 == numi(a);
  case VT_REF_NUM_RATIO:
    return 0 == numrationu(a) &&
      0 != numratiode(a);
  case VT_NUM_REAL:
    return 0.0 == numreal(a);
  case VT_REF_NUM_COMPLEX:
    return 0.0 == numcomplexreal(a) &&
      0.0 == numcompleximag(a);
  case VT_REF_NUM_BIG:
    return numbigref(a)->iszero();
  default:
    Error(vm, "something error won't reach here %d", vttype(a));
    return false;
  }
}

bool SCMMath::toreal(ValueT* z, double* v)
{
  if (isnumi(z))
    *v = numi(z);
  else if (isnumreal(z))
    *v = numreal(z);
  else if (isnumratio(z))
    *v = numrationu(z) / (double)numratiode(z);
  else if (isnumbig(z))
    *v = big2double(numbigref(z));
  else
    return false;
  return true;
}

void SCMMath::complexrect2polarangle(double real,double imag,double *x3)
{
  *x3 = std::atan(imag/real);
}

void SCMMath::complexrect2polarmag(double real,double imag,double *x3)
{
  double ar = std::fabs(real), ai = std::fabs(imag);
  if (ar == 0.0 && ai == 0.0)
  {
    *x3 = 0.0;
    return;
  }
  double m = ar > ai ? ar : ai;
  double r = ar > ai ? ai / ar : ar / ai;
  *x3 = m * std::sqrt(1.0 + r * r);
}

void SCMMath::complexpolar2rectreal(double mag,double angle,double *x3)
{
  *x3 = std::cos(angle)*mag;
}

void SCMMath::complexpolar2rectimag(double mag,double angle,double *x3)
{
  *x3 = std::sin(angle)*mag;
}

void SCMMath::float2ratio(scm_float xr, scm_float *res, scm_int *nu,scm_int *de, int maxiter, scm_float tol)
{
  int sign = xr < 0 ? (xr=-xr,-1) : 1;
  scm_float p0 = std::abs(std::trunc(xr)), q0 = 1, p1 = 1, q1 = 0;
  scm_float a = p0 / q0;
  if (SCMMath::isFloatEqual(xr - a, tol));
  else
  {
    scm_float r = xr - p0;
    while (maxiter-- > 0 && !SCMMath::isFloatEqual(r, 0))
    {
      scm_float xrnxt = 1 / r;
      scm_float q = std::trunc(xrnxt);
      scm_float pnxt = q * p0 + p1, qnxt = q * q0 + q1;
      p1 = p0, q1 = q0;
      p0 = pnxt, q0 = qnxt;
      a = p0 / q0;
      if (SCMMath::isFloatEqual(xr - a, tol))
        break;

      else
        r = xrnxt - q;
    }
  }
  *res = sign * a;
  *nu = p0, *de = sign * q0;
}

static NumBigObj* numbigshlabs(VM* vm, NumBigObj* a, int shift_bits)
{
  if (shift_bits == 0) return a->copy(vm);
  scm_int limb_shifts = shift_bits / INT_BITS;
  int bit_shifts = shift_bits % INT_BITS;
  NumBigObj* res = vm->newnumbig(a->len+limb_shifts+(bit_shifts>0?1:0), a->sign);
  scm_uint carry = 0;
  for (int i = 0; i < a->len; ++i)
  {
    scm_uint cur = a->data[i];
    if (bit_shifts == 0)
      res->data[i + limb_shifts] = cur;
    else
    {
      scm_uint low = (cur << bit_shifts) | carry;
      carry = cur >> (INT_BITS - bit_shifts);
      res->data[i + limb_shifts] = low;
    }
  }
  if (carry > 0)
    res->data[a->len + limb_shifts] = carry;
  return res->normalize();
}

static void numbigshr1absinline(NumBigObj* a)
{
  scm_uint carry = 0;
  for (int i = a->len - 1; i >= 0; --i)
  {
    scm_uint cur = a->data[i];
    scm_uint new_cur = (cur >> 1) | (carry << 63);
    carry = cur & 1ULL;
    a->data[i] = new_cur;
  }
  a->normalize();
}

// (a |= (1 << bit_idx))
static void numbigsetbitinline(NumBigObj* a, scm_int bit_idx)
{
    scm_int limb_idx = bit_idx / INT_BITS;
    scm_int bit_off  = bit_idx % INT_BITS;
    a->data[limb_idx] |= (1ULL << bit_off);
}

NumBigDivRes SCMMath::numbigdivabs(VM* vm, NumBigObj* a, NumBigObj* b)
{
  Assert(vm, !b->iszero(), "big number div: 0");
  int cmp = a->cmpabs(b);
  if (cmp < 0) return {Sbigzero, a->copy(vm)};
  if (cmp == 0)
  {
    NumBigObj* q = vm->newnumbig(1, 1);
    q->data[0] = 1;
    return {q, Sbigzero};
  }
  int bit_len_a = a->bitlength();
  int bit_len_b = b->bitlength();
  int shift = bit_len_a - bit_len_b;
  int q_max_len = (shift / INT_BITS) + 1;
  Sgcvar3(vm, qvt, rvt, svt);
  NumBigObj* quotient = vm->newnumbig(q_max_len, 1);
  setnumbig(qvt, quotient);
  NumBigObj* remainder = a->copy(vm);
  setnumbig(rvt, remainder);
  NumBigObj* shifted_b = numbigshlabs(vm, b, shift);
  setnumbig(svt, shifted_b);
  for (int i = shift; i >= 0; --i)
  {
    if (remainder->cmpabs(shifted_b) >= 0)
    {
      // r = r - shifted_b
      remainder = remainder->subabs(vm, shifted_b);
      setnumbig(rvt, remainder);
      numbigsetbitinline(quotient, i);
    }
    numbigshr1absinline(shifted_b);
  }
  return {quotient->normalize(), remainder->normalize()};
}

static NumBigObj* bigfromstr(VM* vm, const char* digits, int ndigits, int radix, char sign)
{
  Sgcvar5(vm, resultvt, rdvt, tmpvt, radixvt, dvt);
  NumBigObj* bigradix = vm->newnumbig(1, 1);
  bigradix->data[0] = radix;
  setnumbig(radixvt, bigradix);
  NumBigObj* bigd = vm->newnumbig(1, 1);
  setnumbig(dvt, bigd);
  NumBigObj* bigresult = vm->newnumbig(1, 1);
  setnumbig(resultvt, bigresult);
  for (int i = 0; i < ndigits; i++)
  {
    scm_uint d;
    char c = digits[i];
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
    else
      Error(vm, "not supported char in %s", digits);
    bigd->data[0] = d;
    NumBigObj* tmp = bigresult->mul(vm, bigradix);
    tmp->sign = 1;
    setnumbig(tmpvt, tmp);
    bigresult = tmp->addabs(vm, bigd);
    bigresult->sign = 1;
    setnumbig(resultvt, bigresult->normalize());
  }
  bigresult->sign = sign;
  if (bigresult->iszero()) bigresult->sign = 1;
  return bigresult;
}

static void bigvtinline(VM* vm, ValueT* vt)
{
  NumBigObj* big = numbigref(vt);
  big->normalize();
  if (big->len == 1 && big->data[0] <= (scm_uint)SCM_INT_MAX)
  {
    scm_int val = (scm_int)big->data[0];
    if (big->sign < 0) val = -val;
    setnumi(vt, val);
  }
}

static void bigsetvt(VM* vm, ValueT* vt, NumBigObj* big)
{
  big->normalize();
  if (big->len == 1 && big->data[0] <= (scm_uint)SCM_INT_MAX)
  {
    scm_int val = (scm_int)big->data[0];
    if (big->sign < 0) val = -val;
    setnumi(vt, val);
  }
  else
    setnumbig(vt, big);
}

static NumBigObj* numbigfromi(VM* vm, scm_int val)
{
  char sign = 1;
  scm_uint uval;
  if (val < 0)
  {
    sign = -1;
    uval = -val;
  }
  else
    uval = val;
  NumBigObj* num = vm->newnumbig(1, sign);
  num->data[0] = uval;
  return num;
}

bool SCMMath::exactp(ValueT* p)
{
  return isnumi(p) || isnumratio(p) || isnumbig(p);
}

bool SCMMath::numlessp(ValueT* p1, ValueT* p2)
{
  switch(vttype(p1)) {
  case VT_NUM_INTEGER:
    if (numi(p1) >= numi(p2))
      return false;
    break;

  case VT_NUM_REAL:
    if (numreal(p1) > numreal(p2) || SCMMath::isFloatEqual(numreal(p1), numreal(p2)))
      return false;
    break;

  case VT_REF_NUM_RATIO:{
    if (numratiode(p1) == numratiode(p2) && numrationu(p1) >= numrationu(p2))
      return false;

    else if (numrationu(p1) == numrationu(p2) && numratiode(p1) <= numratiode(p2))
      return false;

    else {
      // set de equal
      scm_int de = SCMMath::lcm(numratiode(p1), numratiode(p2));
      scm_int dp1 = de / numratiode(p1);
      scm_int dp2 = de / numratiode(p2);

      if (numrationu(p1) * dp1 >= numrationu(p2) * dp2)
        return false;
    }
    break;
  }

  case VT_REF_NUM_COMPLEX:
    if (!SCMMath::isFloatEqual(numcompleximag(p1), 0) ||
        !SCMMath::isFloatEqual(numcompleximag(p2), 0))
      return false;

    if (numcomplexreal(p1) > numcomplexreal(p2) ||
        SCMMath::isFloatEqual(numcomplexreal(p1), numcomplexreal(p2)))
      return false;
    break;

  case VT_REF_NUM_BIG:
    if (numbigref(p1)->cmp(numbigref(p2)) >= 0)
      return false;
    break;
  }
  return true;
}

bool SCMMath::numequalp(ValueT* p1, ValueT* p2)
{
  switch(vttype(p1)) {
  case VT_NUM_INTEGER:
    if (numi(p1) != numi(p2))
      return false;
    break;

  case VT_NUM_REAL:
    if (!SCMMath::isFloatEqual(numreal(p1), numreal(p2)))
      return false;
    break;

  case VT_REF_NUM_RATIO:{
    if (numratiode(p1) == numratiode(p2)) {
      if (numrationu(p1) != numrationu(p2))
        return false;
    }
    else {
      // set de equal
      scm_int de = SCMMath::lcm(numratiode(p1), numratiode(p2));
      scm_int dp1 = de / numratiode(p1);
      scm_int dp2 = de / numratiode(p2);

      if (numrationu(p1) * dp1 != numrationu(p2) * dp2)
        return false;
    }
    break;
  }

  case VT_REF_NUM_COMPLEX:
    if (!SCMMath::isFloatEqual(numcomplexreal(p1), numcomplexreal(p2)) ||
        !SCMMath::isFloatEqual(numcompleximag(p1), numcompleximag(p2)))
      return false;
    break;

  case VT_REF_NUM_BIG:
    if (numbigref(p1)->cmp(numbigref(p2)) != 0)
      return false;
    break;
  }
  return true;
}

bool SCMMath::numbiggerp(ValueT* p1, ValueT* p2)
{
  switch(vttype(p1)) {
  case VT_NUM_INTEGER:
    if (numi(p1) <= numi(p2))
      return false;
    break;

  case VT_NUM_REAL:
    if (numreal(p1) <= numreal(p2))
      return false;
    break;

  case VT_REF_NUM_RATIO:{
    if (numratiode(p1) == numratiode(p2) && numrationu(p1) <= numrationu(p2))
      return false;

    else if (numrationu(p1) == numrationu(p2) && numratiode(p1) >= numratiode(p2))
      return false;

    else {
      // set de equal
      scm_int de = SCMMath::lcm(numratiode(p1), numratiode(p2));
      scm_int dp1 = de / numratiode(p1);
      scm_int dp2 = de / numratiode(p2);

      if (numrationu(p1) * dp1 <= numrationu(p2) * dp2)
        return false;
    }
    break;
  }

  case VT_REF_NUM_COMPLEX:
    if (!SCMMath::isFloatEqual(numcompleximag(p1), 0) ||
        !SCMMath::isFloatEqual(numcompleximag(p2), 0))
      return false;

    if (numcomplexreal(p1) <= numcomplexreal(p2))
      return false;
    break;

  case VT_REF_NUM_BIG:
    if (numbigref(p1)->cmp(numbigref(p2)) <= 0)
      return false;
    break;
  }
  return true;
}

bool SCMMath::isFloatEqual(scm_float a, scm_float b)
{
  return std::abs(a - b) <= DBL_EPSILON;
}

NumBigObj* SCMMath::lcm(VM* vm, NumBigObj* a, NumBigObj* b)
{
  Sgcvar2(vm, tmpvt, tmp2vt);
  NumBigObj* tmp = SCMMath::gcd(vm, a, b);
  setnumbig(tmpvt, tmp);
  NumBigObj* tmp2 = a->mulabs(vm, b);
  setnumbig(tmp2vt, tmp2);
  return SCM::numbigdiv(vm, tmp2, tmp);
}

scm_int SCMMath::lcm(scm_int a, scm_int b)
{
  return std::abs(a * b / gcd(a, b));
}

NumBigObj* SCMMath::gcd(VM* vm, NumBigObj* a, NumBigObj* b)
{
  Sgcvar2(vm, avt, bvt);
  while (!b->iszero())
  {
    NumBigObj* tmp = SCM::numbigmod(vm, a, b);
    a = b;
    b = tmp;
    setnumbig(avt, a);
    setnumbig(bvt, b);
  }
  return a;
}

scm_int SCMMath::gcd(scm_int a, scm_int b)
{
  a = std::abs(a);
  b = std::abs(b);
  while (b != 0)
  {
    scm_int tmp = a % b;
    a = b;
    b = tmp;
  }
  return a;
}

void SCMMath::numbigdiv(VM* vm, NumBigObj* a, NumBigObj* b, NumBigObj** q, NumBigObj** r)
{
  NumBigDivRes res = SCMMath::numbigdivabs(vm, a, b);
  res.quot->sign = (a->sign == b->sign) ? 1 : -1;
  *q = res.quot;
  *r = res.rem;
}

static void simplifyratio(ValueT* v)
{
  // assert isnumratio()
  if (numratiode(v) == 1)
    setnumi(v, numrationu(v));
  else if (numratiode(v) == -1)
    setnumi(v, -1 * numrationu(v));
}

static void simplifycomplex(ValueT* v)
{
  // assert isnumcomplex()
  if (0.0 == numcompleximag(v))
    setnumreal(v, numcomplexreal(v));
}

static void numuptoratio(VM* vm, ValueT* a)
{
  AssertVT(vm, isnumi(a), a, "not a number");
  scm_int si = numi(a);
  setnumratio(a, Sr2(vm, NumRatioObj, si, 1));
}

static void numuptoreal(VM* vm, ValueT* a)
{
  if (isnumi(a))
    setnumreal(a, numi(a));
  else if (isnumratio(a))
    setnumreal(a, numrationu(a) / (double)numratiode(a));
  else
    ErrorVT(vm, a, "not integer or ratio");
}

static void numtocomplex(VM*vm, ValueT* a)
{
  if (isnumi(a))
    setnumcomplex(a, Sr2(vm, NumComplexObj, numi(a), 0));
  else if (isnumratio(a))
    setnumcomplex(a, Sr2(vm, NumComplexObj, numrationu(a) / (double)numratiode(a), 0));
  else if (isnumreal(a))
    setnumcomplex(a, Sr2(vm, NumComplexObj, numreal(a), 0));
  else
    ErrorVT(vm, a, "not integer or ratio or real");
}

static void numsynctype(VM* vm, ValueT* a1, ValueT* a2, ValueT* a1p, ValueT* a2p)
{
  *a1 = a1p;
  *a2 = a2p;
  if (vttype(a1) == vttype(a2)) return;
  switch(vttype(a1)) {
  case VT_NUM_INTEGER:
    switch(vttype(a2)) {
    case VT_REF_NUM_RATIO:numuptoratio(vm, a1);break;
    case VT_NUM_REAL: numuptoreal(vm, a1); break;
    case VT_REF_NUM_COMPLEX: numtocomplex(vm, a1); break;
    case VT_REF_NUM_BIG: setnumbig(a1, numbigfromi(vm, numi(a1))); break;
    }
    break;
  case VT_REF_NUM_RATIO:
    switch(vttype(a2)) {
    case VT_NUM_INTEGER: numuptoratio(vm, a2); break;
    case VT_NUM_REAL: numuptoreal(vm, a1); break;
    case VT_REF_NUM_COMPLEX: numtocomplex(vm, a1); break;
    case VT_REF_NUM_BIG: setnumreal(a2, big2double(numbigref(a2))); break;
    }
    break;
  case VT_NUM_REAL:
    switch(vttype(a2)) {
    case VT_NUM_INTEGER: numuptoreal(vm, a2); break;
    case VT_REF_NUM_RATIO: numuptoreal(vm, a2); break;
    case VT_REF_NUM_COMPLEX: numtocomplex(vm, a1); break;
    case VT_REF_NUM_BIG: setnumreal(a2, big2double(numbigref(a2))); break;
    }
    break;
  case VT_REF_NUM_COMPLEX:
    switch(vttype(a2)) {
    case VT_NUM_INTEGER: numtocomplex(vm, a2); break;
    case VT_REF_NUM_RATIO: numtocomplex(vm, a2); break;
    case VT_NUM_REAL: numtocomplex(vm, a2); break;
    case VT_REF_NUM_BIG: setnumreal(a1, big2double(numbigref(a1))); break;
    }
    break;
  case VT_REF_NUM_BIG:
    switch(vttype(a2)) {
    case VT_NUM_INTEGER: setnumbig(a2, numbigfromi(vm, numi(a2))); break;
    case VT_REF_NUM_RATIO: case VT_NUM_REAL: setnumreal(a1, big2double(numbigref(a1))); break;
    case VT_REF_NUM_COMPLEX: numtocomplex(vm, a1); break;
    }
    break;
  }
}

static bool numaddtoverflow(scm_int a, scm_int b, scm_int* out)
{
  if (b > 0 && a > SCM_INT_MAX - b) return true;
  if (b < 0 && a < SCM_INT_MIN - b) return true;
  *out = a + b;
  return false;
}

static bool numsubtoverflow(scm_int a, scm_int b, scm_int* out)
{
  if (b > 0 && a < SCM_INT_MIN + b) return true;
  if (b < 0 && a > SCM_INT_MAX + b) return true;
  *out = a - b;
  return false;
}

static bool nummultooverflow(scm_int a, scm_int b, scm_int* out)
{
  if (a > 0) {
    if (b > 0) {
      if (a > SCM_INT_MAX / b) return true;
    } else {
      if (b < SCM_INT_MIN / a) return true;
    }
  } else {
    if (b > 0) {
      if (a < SCM_INT_MIN / b) return true;
    } else {
      if (a != 0 && b < SCM_INT_MAX / a) return true;
    }
  }
  *out = a * b;
  return false;
}

static void numbigfromfixnums(VM* vm, ValueT* b1, ValueT* b2, scm_int a, scm_int b)
{
  setnumbig(b1, numbigfromi(vm, a));
  setnumbig(b2, numbigfromi(vm, b));
}

static void numadd(VM* vm, ValueT* out, ValueT* a1, ValueT* a2)
{
  Sgcvar2(vm, a1p, a2p);
  numsynctype(vm, a1p, a2p, a1, a2);
  a1 = a1p, a2 = a2p;

  switch(vttype(a1)) {
  case VT_NUM_INTEGER: {
    scm_int r, a1i = numi(a1), a2i = numi(a2);
    if (!numaddtoverflow(a1i, a2i, &r))
      setnumi(out, r);
    else
    {
      Sgcvar2(vm, b1, b2);
      numbigfromfixnums(vm, b1, b2, a1i, a2i);
      bigsetvt(vm, out, numbigref(b1)->add(vm, numbigref(b2)));
    }
    break;
  }
  case VT_REF_NUM_RATIO:{
    scm_int de = SCMMath::lcm(numratiode(a1), numratiode(a2));
    scm_int dp1 = de / numratiode(a1);
    scm_int dp2 = de / numratiode(a2);
    setnumratio(out, Sr2(vm, NumRatioObj, numrationu(a1) * dp1 + numrationu(a2) * dp2, de));
    simplifyratio(out);
    break;
  }
  case VT_NUM_REAL:
    setnumreal(out, numreal(a1) + numreal(a2));
    break;
  case VT_REF_NUM_COMPLEX:
    setnumcomplex(out, Sr2(vm, NumComplexObj,
                           numcomplexreal(a1) + numcomplexreal(a2),
                           numcompleximag(a1) + numcompleximag(a2)));
    simplifycomplex(out);
    break;
  case VT_REF_NUM_BIG:
    bigsetvt(vm, out, numbigref(a1)->add(vm, numbigref(a2)));
    break;
  }
}

static void numsubtract(VM* vm, ValueT* out, ValueT* a1, ValueT* a2)
{
  switch(vttype(a1)) {
  case VT_NUM_INTEGER: {
    scm_int a1i = numi(a1), a2i = numi(a2), r;
    if (!numsubtoverflow(a1i, a2i, &r))
      setnumi(out, r);
    else
    {
      Sgcvar2(vm, b1, b2);
      numbigfromfixnums(vm, b1, b2, a1i, a2i);
      bigsetvt(vm, out, numbigref(b1)->sub(vm, numbigref(b2)));
    }
    break;
  }
  case VT_REF_NUM_RATIO:{
    scm_int de = SCMMath::lcm(numratiode(a1), numratiode(a2));
    scm_int dp1 = de / numratiode(a1);
    scm_int dp2 = de / numratiode(a2);
    setnumratio(out, Sr2(vm, NumRatioObj, numrationu(a1) * dp1 - numrationu(a2) * dp2, de));
    simplifyratio(out);
    break;
  }
  case VT_NUM_REAL:
    setnumreal(out, numreal(a1) - numreal(a2));
    break;
  case VT_REF_NUM_COMPLEX:
    setnumcomplex(out, Sr2(vm, NumComplexObj,
                           numcomplexreal(a1) - numcomplexreal(a2),
                           numcompleximag(a1) - numcompleximag(a2)));
    simplifycomplex(out);
    break;
  case VT_REF_NUM_BIG:
    bigsetvt(vm, out, numbigref(a1)->sub(vm, numbigref(a2)));
    break;
  }
}

static void nummultiply(VM* vm, ValueT* out, ValueT* a1, ValueT* a2)
{
  Sgcvar2(vm, a1p, a2p);
  numsynctype(vm, a1p, a2p, a1, a2);
  a1 = a1p, a2 = a2p;

  switch(vttype(a1)) {
  case VT_NUM_INTEGER: {
    scm_int a1i = numi(a1), a2i = numi(a2), r;
    if (!nummultooverflow(a1i, a2i, &r))
      setnumi(out, r);
    else
    {
      Sgcvar2(vm, b1, b2);
      numbigfromfixnums(vm, b1, b2, a1i, a2i);
      bigsetvt(vm, out, numbigref(b1)->mul(vm, numbigref(b2)));
    }
    break;
  }
  case VT_REF_NUM_RATIO:
    setnumratio(out, Sr2(vm, NumRatioObj,
                       numrationu(a1) * numrationu(a2) ,
                       numratiode(a1) * numratiode(a2)));
    simplifyratio(out);
    break;
  case VT_NUM_REAL:
    setnumreal(out, numreal(a1) * numreal(a2));
    break;
  case VT_REF_NUM_COMPLEX: {
    double re1 = numcomplexreal(a1), re2 = numcomplexreal(a2);
    double ig1 = numcompleximag(a1), ig2 = numcompleximag(a2);
    setnumcomplex(out, Sr2(vm, NumComplexObj, (re1 * re2 - ig1 * ig2), re1 * ig2 + ig1 * re2));
    simplifycomplex(out);
    break;
  }
  case VT_REF_NUM_BIG:
    bigsetvt(vm, out, numbigref(a1)->mul(vm, numbigref(a2)));
    break;
  }
}

static void numdivide(VM* vm, ValueT* out, ValueT* a1, ValueT* a2)
{
  Sgcvar2(vm, a1p, a2p);
  numsynctype(vm, a1p, a2p, a1, a2);
  a1 = a1p, a2 = a2p;

  switch(vttype(a1)) {
  case VT_NUM_INTEGER: {
    if (0 == numi(a1) % numi(a2))
      setnumi(out, numi(a1) / numi(a2));

    else
    {
      scm_int gd = SCMMath::gcd(numi(a1), numi(a2));
      setnumratio(out, Sr2(vm, NumRatioObj, numi(a1) / gd, numi(a2) / gd));
      simplifyratio(out);
    }
    break;
  }
  case VT_REF_NUM_RATIO:
    setnumratio(out, Sr2(vm, NumRatioObj,
                         numrationu(a1) * numratiode(a2) ,
                         numratiode(a1) * numrationu(a2)));
    simplifyratio(out);
    break;
  case VT_NUM_REAL:
    setnumreal(out, numreal(a1) / numreal(a2));
    break;
  case VT_REF_NUM_COMPLEX: {
    double re1 = numcomplexreal(a1), re2 = numcomplexreal(a2);
    double ig1 = numcompleximag(a1), ig2 = numcompleximag(a2);

    // Smith's algorithm: scale by the larger component to avoid
    // overflow/underflow in the common denominator c*c + d*d
    double real, imag;
    if (std::fabs(re2) >= std::fabs(ig2))
    {
      double r = ig2 / re2;
      double den = re2 + ig2 * r;
      real = (re1 + ig1 * r) / den;
      imag = (ig1 - re1 * r) / den;
    }
    else
    {
      double r = re2 / ig2;
      double den = re2 * r + ig2;
      real = (re1 * r + ig1) / den;
      imag = (ig1 * r - re1) / den;
    }
    setnumcomplex(out, Sr2(vm, NumComplexObj, real, imag));
    simplifycomplex(out);
    break;
  }
  case VT_REF_NUM_BIG: {
    NumBigObj* q = NULL;
    NumBigObj* rem = NULL;
    SCMMath::numbigdiv(vm, numbigref(a1), numbigref(a2), &q, &rem);
    if (rem->iszero())
      bigsetvt(vm, out, q);
    else
    {
      double d1 = big2double(numbigref(a1));
      double d2 = big2double(numbigref(a2));
      setnumreal(out, d1 / d2);
    }
    break;
  }
  }
}

static ValueT scm_stub_numerator(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "numerator";
  AssertVT(vm, isnumratio(p), p, "%s: not a ratio num", METHOD_NAME);
  ValueT out;
  setnumi(&out, numrationu(p));
  return out;
}

static ValueT scm_stub_denominator(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "denominator";
  Assert(vm, isnumratio(p), "%s: not a ratio num", METHOD_NAME);
  ValueT out;
  setnumi(&out, numratiode(p));
  return out;
}

static ValueT scm_stub_quotient(VM* vm, ValueT* p1, ValueT* p2)
{
  static const char* METHOD = "quotient";
  AssertVT(vm, isnumi(p1) || isnumbig(p1), p1, "%s: needs an integer", METHOD);
  AssertVT(vm, isnumi(p2) || isnumbig(p2), p2, "%s: needs an integer", METHOD);
  if (isnumi(p1) && isnumi(p2))
  {
    AssertVT(vm, numi(p2) != 0, p2, "%s: non-zero value needed", METHOD);
    ValueT out;
    setnumi(&out, numi(p1) / numi(p2));
    return out;
  }
  Sgcvar2(vm, n1, n2);
  numsynctype(vm, n1, n2, p1, p2);
  AssertVT(vm, !numbigref(n2)->iszero(), n2, "%s: non-zero value needed", METHOD);
  char sign_a = numbigref(n1)->sign;
  char sign_b = numbigref(n2)->sign;
  NumBigObj* q = NULL;
  NumBigObj* r = NULL;
  SCMMath::numbigdiv(vm, numbigref(n1), numbigref(n2), &q, &r);
  if (q->iszero()) q->sign = 1;
  ValueT out;
  bigsetvt(vm, &out, q);
  return out;
}

static ValueT scm_stub_modulo(VM* vm, ValueT* p1, ValueT* p2)
{
  static const char* METHOD = "modulo";
  AssertVT(vm, isnumi(p1) || isnumbig(p1), p1, "%s: needs an integer", METHOD);
  AssertVT(vm, isnumi(p2) || isnumbig(p2), p2, "%s: needs an integer", METHOD);
  if (isnumi(p1) && isnumi(p2))
  {
    AssertVT(vm, numi(p2) != 0, p2, "%s: non-zero value needed", METHOD);
    scm_int a = numi(p1), b = numi(p2);
    scm_int rem = a % b;
    if (rem != 0 && (rem < 0) != (b < 0))
      rem += b;
    ValueT out;
    setnumi(&out, rem);
    return out;
  }
  Sgcvar4(vm, n1, n2, rvt, tmpvt);
  numsynctype(vm, n1, n2, p1, p2);
  AssertVT(vm, !numbigref(n2)->iszero(), n2, "%s: non-zero value needed", METHOD);
  char sign_a = numbigref(n1)->sign;
  char sign_b = numbigref(n2)->sign;
  NumBigObj* r = SCM::numbigmod(vm, numbigref(n1), numbigref(n2));
  setnumbig(rvt, r);
  if (!r->iszero() && sign_a != sign_b)
  {
    // r = |b| - r, then set sign to sign_b
    NumBigObj* abs_b = numbigref(n2)->copy(vm);
    abs_b->sign = 1;
    setnumbig(tmpvt, abs_b);
    abs_b->normalize();
    r = abs_b->subabs(vm, r);
    r->sign = sign_b;
    setnumbig(rvt, r);
  }
  else
    r->sign = sign_b;
  if (r->iszero()) r->sign = 1;
  ValueT out;
  bigsetvt(vm, &out, r);
  return out;
}

static ValueT scm_stub_remainder(VM* vm, ValueT* p1, ValueT* p2)
{
  static const char* METHOD = "remainder";
  AssertVT(vm, isnumi(p1) || isnumbig(p1), p1, "%s: needs an integer", METHOD);
  AssertVT(vm, isnumi(p2) || isnumbig(p2), p2, "%s: needs an integer", METHOD);
  if (isnumi(p1) && isnumi(p2))
  {
    AssertVT(vm, numi(p2) != 0, p2, "%s: non-zero value needed", METHOD);
    scm_int rem = abs(numi(p1)) % abs(numi(p2));
    if (numi(p1) < 0)
      rem = -rem;
    ValueT out;
    setnumi(&out, rem);
    return out;
  }
  // Bignum path: remainder has sign of dividend
  Sgcvar2(vm, n1, n2);
  numsynctype(vm, n1, n2, p1, p2);
  AssertVT(vm, !numbigref(n2)->iszero(), n2, "%s: non-zero value needed", METHOD);
  NumBigObj* r = SCM::numbigmod(vm, numbigref(n1), numbigref(n2));
  if (!r->iszero())
    r->sign = numbigref(n1)->sign;
  ValueT out;
  bigsetvt(vm, &out, r);
  return out;
}

static ValueT scm_stub_less(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "<";
  AssertArg(vm, isnumber(p1), METHOD_NAME , p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (!SCMMath::numlessp(n, n1))
      return Sfalseref;
    *n = n1;
  }
  return Strueref;
}

static ValueT scm_stub_max(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "max";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (SCMMath::numbiggerp(n1, n))
      n = n1;
  }
  return n;
}

static ValueT scm_stub_min(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "min";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (SCMMath::numlessp(n1, n))
      n = n1;
  }
  return n;
}

static ValueT scm_stub_bigger(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = ">";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (!SCMMath::numbiggerp(n, n1))
      return Sfalseref;
    *n = n1;
  }
  return Strueref;
}

static ValueT scm_stub_biggereq(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = ">=";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (SCMMath::numlessp(n, n1))
      return Sfalseref;
    *n = n1;
  }
  return Strueref;
}

static ValueT scm_stub_lesseq(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "<=";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (SCMMath::numbiggerp(n, n1))
      return Sfalseref;
    *n = n1;
  }
  return Strueref;
}

static ValueT scm_stub_add(VM* vm, ValueT* args)
{
  static const char* METHOD_NAME = "+";
  if (isnull(args))
  {
    ValueT out;
    setnumi(&out, 0);
    return out;
  }

  else
  {
    ValueT* n = Scar(args);
    AssertArg(vm, isnumber(n), METHOD_NAME, n, " not a number");
    args = Scdr(args);
    Sgcvar2(vm, out, out2);
    *out = n;
    PAIR_FOR(p, args)
    {
      ValueT* n = Scar(p);
      AssertArg(vm, isnumber(n), METHOD_NAME, n, " not a number");
      numadd(vm, out2, out, n);
      *out = out2;
    }
    return out;
  }
}

static ValueT scm_stub_subtract(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "-";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  Sgcvar3(vm, out, np1, np2);
  if (isnull(args))
  {
    ValueT p2;
    setnumi(&p2, 0);
    Sgcvar2(vm, np1, np2);
    numsynctype(vm, np1, np2, p1, &p2);
    numsubtract(vm, out, np2, np1);
    return out;
  }
  else
  {
    ValueT o2;
    *out = p1;
    PAIR_FOR(p, args)
    {
      ValueT* n = Scar(p);
      AssertArg(vm, isnumber(n), METHOD_NAME, n, " not a number");
      numsynctype(vm, np1, np2, out, n);
      numsubtract(vm, out, np1, np2);
    }
    return out;
  }
}

static ValueT scm_stub_multiply(VM* vm, ValueT* args)
{
  static const char* METHOD_NAME = "*";
  if (isnull(args))
  {
    ValueT out;
    setnumi(&out, 1);
    return out;
  }

  else
  {
    ValueT* n = Scar(args);
    AssertArg(vm, isnumber(n), METHOD_NAME, n, " not a number");
    args = Scdr(args);
    Sgcvar2(vm, out, out2);
    *out = n;
    PAIR_FOR(p, args)
    {
      ValueT* n = Scar(p);
      AssertArg(vm, isnumber(n), METHOD_NAME, n, " not a number");
      nummultiply(vm, out2, out, n);
      *out = out2;
    }
    return out;
  }
}

static ValueT scm_stub_divide(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "/";
  AssertArg(vm, isnumber(p1), METHOD_NAME , p1, " not a number");
  Sgcvar2(vm, out, out2);
  *out = p1;
  PAIR_FOR(p, args)
  {
    ValueT* n = Scar(p);
    AssertArg(vm, isnumber(n), METHOD_NAME , n, " not a number");
    AssertArg(vm, !SCMMath::isnumzero(vm, n), METHOD_NAME , n, " can not be zero");
    numdivide(vm, out2, out, n);
    *out = out2;
  }
  return out;
}

static ValueT scm_stub_gcd(VM* vm, ValueT* args)
{
  static const char* METHOD = "gcd";
  ValueT out;
  if (isnull(args))
    setnumi(&out, 0);
  else
  {
    ValueT* n = Scar(args);
    AssertVT(vm, isnumi(n) || isnumbig(n), n, "%s: not a number", METHOD);
    args = Scdr(args);
    Sgcvar1(vm, outv);
    *outv = n;
    bool hasbig = isnumbig(n);
    PAIR_FOR(p, args)
    {
      n = Scar(p);
      AssertVT(vm, isnumi(n) || isnumbig(n), n, "%s: not a number", METHOD);
      if (isnumbig(n)) hasbig = true;
      if (hasbig)
      {
        Sgcvar2(vm, a, b);
        numsynctype(vm, a, b, outv, n);
        if (isnumi(a) && isnumi(b))
        {
          setnumi(outv, SCMMath::gcd(std::abs(numi(a)), std::abs(numi(b))));
          hasbig = false;
        }
        else
          setnumbig(outv, SCMMath::gcd(vm, numbigref(a), numbigref(b)));
      }
      else
        setnumi(outv, SCMMath::gcd(std::abs(numi(outv)), std::abs(numi(n))));
    }
    if (isnumbig(outv))
    {
      numbigref(outv)->sign = 1;
      bigvtinline(vm, outv);
    }
    else
      setnumi(outv, std::abs(numi(outv)));
    out = *outv;
  }
  return out;
}

static ValueT scm_stub_lcm(VM* vm, ValueT* args)
{
  Sgcvar6(vm, outv, a, b, prodvt, gv, pv);
  static const char* METHOD = "lcm";
  ValueT out;
  if (isnull(args))
    setnumi(&out, 1);
  else
  {
    ValueT* n = Scar(args);
    AssertVT(vm, isnumi(n) || isnumbig(n), n, "%s: not a number", METHOD);
    args = Scdr(args);
    *outv = *n;
    bool hasbig = isnumbig(n);
    PAIR_FOR(p, args)
    {
      AssertVT(vm, ispair(p), p, "%s: not a pair", METHOD);
      n = Scar(p);
      AssertVT(vm, isnumi(n) || isnumbig(n), n, "%s: not a number", METHOD);
      if (isnumbig(n)) hasbig = true;
      if (hasbig)
      {
        numsynctype(vm, a, b, outv, n);
        NumBigObj* tmp = SCMMath::lcm(vm, numbigref(a), numbigref(b));
        bigsetvt(vm, outv, tmp);
      }
      else
        setnumi(outv, SCMMath::lcm(std::abs(numi(outv)), std::abs(numi(n))));
    }
    // Ensure result is positive
    if (isnumbig(outv))
      numbigref(outv)->sign = 1;
    else
      setnumi(outv, std::abs(numi(outv)));
    out = *outv;
  }
  return out;
}

static ValueT scm_stub_positivep(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "positive?";
  AssertArg(vm, isnumber(p), METHOD_NAME, p, " not a number");
  ValueT p2;
  setnumi(&p2, 0);
  Sgcvar2(vm, np, np2);
  numsynctype(vm, np, np2, p, &p2);
  return frombool(SCMMath::numbiggerp(np, np2));
}

static ValueT scm_stub_exactp(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "exact?";
  AssertArg(vm, isnumber(p), METHOD_NAME, p, " not a number");
  return frombool(SCMMath::exactp(p));
}

static ValueT scm_stub_rationalize(VM* vm, ValueT* x, ValueT* y)
{
  static const char* METHOD_NAME = "rationalize";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  AssertArg(vm, isnumber(y), METHOD_NAME, y, "not a number");
  scm_float xr = 0;
  if (!SCMMath::toreal(x, &xr))
    ErrorVT(vm, x, "%s: unsupported num", METHOD_NAME);
  scm_float tol = 0;
  if (!SCMMath::toreal(y, &tol))
    ErrorVT(vm, y, "%s: unsupported num", METHOD_NAME);
  AssertVT(vm, tol >= 0, y, "%s: tolerance must be positive", METHOD_NAME);

  scm_int sign = xr < 0 ? -1 : 1;
  xr = std::abs(xr);
  scm_float a=0;
  scm_int _1,_2;
  SCMMath::float2ratio(xr, &a, &_1, &_2, 10, tol);
  ValueT out;
  setnumreal(&out, sign*a);
  return out;
}

#define SCM_STUB_SIN_COS(name) static                       \
 ValueT scm_stub_## name(VM* vm, ValueT* z) {               \
 AssertArg(vm, isnumber(z), #name, z, " not a number");     \
 ValueT out;                                                \
 if (isnumi(z)) setnumreal(&out, std:: name (numi(z)));  \
 else if (isnumreal(z))                                     \
   setnumreal(&out, std:: name (numreal(z)));            \
 else if (isnumratio(z))                                    \
   setnumreal(&out, std:: name (numrationu(z)            \
                                   /                        \
                                   (double)numratiode(z))); \
 else ErrorVT(vm, z, #name ": parameter error");            \
 return out;                                                \
}

SCM_STUB_SIN_COS(asin)
SCM_STUB_SIN_COS(acos)
SCM_STUB_SIN_COS(tan)
SCM_STUB_SIN_COS(cos)
SCM_STUB_SIN_COS(sin)
SCM_STUB_SIN_COS(log)
SCM_STUB_SIN_COS(exp)

static void scm_int_exact_power(scm_int base, scm_int exp, ValueT* out)
{
  scm_int r = 1, b = base, e = exp;
  while (e > 0)
  {
    if (e & 1)
      r *= b;
    e >>= 1;
    if (e > 0)
      b *= b;
  }
  setnumi(out, r);
}

static NumBigObj* bigfromuint(VM* vm, scm_uint v, char sign)
{
  NumBigObj* tmp = vm->newnumbig(1, sign);
  tmp->data[0] = v;
  return tmp;
}

static ValueT scm_stub_expt(VM* vm, ValueT* z1, ValueT* z2)
{
  static const char* METHOD = "expt";
  AssertVT(vm, isnumber(z1), z1, "%s: not a number", METHOD);
  AssertVT(vm, isnumber(z2), z2, "%s: not a number", METHOD);

  // Integer exponentiation: integer base, non-negative integer exponent
  if ((isnumi(z1) || isnumbig(z1)) && isnumi(z2))
  {
    scm_int e = numi(z2);
    // Quick shortcuts for base = 1 or -1
    if (isnumi(z1))
    {
      scm_int bv = numi(z1);
      if (bv == 1 || bv == -1)
      {
        ValueT out;
        setnumi(&out, (bv == -1 && (e & 1)) ? -1 : 1);
        return out;
      }
    }
    if (e >= 0)
    {
      // Binary exponentiation with bignum arithmetic
      Sgcvar2(vm, result, base_v);
      if (isnumi(z1))
        setnumbig(base_v, numbigfromi(vm, numi(z1)));
      else
        setnumbig(base_v, numbigref(z1));
      setnumbig(result, bigfromuint(vm, 1, 1));

      scm_int exp = e;
      while (exp > 0)
      {
        if (exp & 1)
        {
          NumBigObj* r = numbigref(result)->mul(vm, numbigref(base_v));
          setnumbig(result, r);
        }
        exp >>= 1;
        if (exp > 0)
        {
          NumBigObj* b2 = numbigref(base_v)->mul(vm, numbigref(base_v));
          setnumbig(base_v, b2);
        }
      }
      ValueT out;
      bigsetvt(vm, &out, numbigref(result));
      return out;
    }
    // e < 0: fall through to double path
  }
  double z1r = 0, z2r = 0;
  if (!SCMMath::toreal(z1, &z1r) || !SCMMath::toreal(z2, &z2r))
    ErrorVT(vm, z1, "%s: dont support", METHOD);
  ValueT out;
  setnumreal(&out, std::pow(z1r, z2r));
  return out;
}

#define check_cond(cond) if (!(cond)) return false;
#define digit_value(c) (((c)<='9') ? ((c) - '0') : ((std::tolower(c) - 'a') + 10))
static const char number_chars[] = "0123456789abcdef";

static bool scm_readsuffix(VM* vm, ReadNumState* state, scm_float val)
{
  bool negativep = false;
  switch(state->c) {
  case '-': negativep = true;
  case '+': state->next(); break;
  }
  switch(state->c) {
  case CASE_09DIGIT:case CASE_AFDIGIT:{
    scm_int exp = 0;
    do {
      int digit = digit_value(state->c);
      if (digit < 0 || digit >= state->radix)
        return false;
      exp = exp * state->radix + digit;
      state->next();
    } while (std::isxdigit(state->c));
    state->n.setreal(val * std::pow(10, negativep ? -exp : exp));
    return true;
  }
  default: return false;
  }
}

static bool scm_readuinteger(VM* vm, ReadNumState* state)
{
  scm_uint uval = 0;
  bool overflow = false;
  scm_uint maxpre = ((scm_uint)SCM_INT_MAX - 9) / state->radix;
  char digits[1024]={0};
  int ndigits = 0;
  do {
    int digit = digit_value(state->c);
    if (digit < 0 || digit >= state->radix)
      return false;
    if (!overflow)
    {
      if (uval > maxpre)
      {
        // Flush accumulated uval into digits array
        char tmp[64] = {0};
        int ntmp = 0;
        scm_uint tv = uval;
        if (tv == 0)
          tmp[ntmp++] = '0';
        else
        {
          while (tv > 0)
          {
            tmp[ntmp++] = number_chars[tv % state->radix];
            tv /= state->radix;
          }
        }
        for (int i = ntmp - 1; i >= 0; i--)
          digits[ndigits++] = tmp[i];
        overflow = true;
        if (ndigits < (int)sizeof(digits))
          digits[ndigits++] = number_chars[digit];
        else
          Error(vm, "too long int, not support yet, %s", digits);
      }
      else
        uval = uval * state->radix + digit;
    }
    else
    {
      if (ndigits < (int)sizeof(digits))
        digits[ndigits++] = number_chars[digit];
      else
        Error(vm, "too long int, not support yet, %s", digits);
    }
    state->next();
  } while (std::isxdigit(state->c) &&
           !(state->radix == 10 && (state->c == 'e' || state->c == 'E')));
  if (state->c == '#')
  {
    do {
      if (!overflow)
      {
        if (uval > maxpre)
        {
          char tmp[64] = {0};
          int ntmp = 0;
          scm_uint tv = uval;
          if (tv == 0)
            tmp[ntmp++] = '0';
          else
          {
            while (tv > 0)
            {
              tmp[ntmp++] = number_chars[tv % state->radix];
              tv /= state->radix;
            }
          }
          for (int i = ntmp - 1; i >= 0; i--)
            digits[ndigits++] = tmp[i];
          overflow = true;
        }
        else
          uval = uval * state->radix;
      }
      else
      {
        if (ndigits < (int)sizeof(digits))
          digits[ndigits++] = '0';
        else
          Error(vm, "too long int, not support yet, %s", digits);
      }
      state->next();
    } while (state->c == '#');
  }
  if (overflow)
    state->bigval = bigfromstr(vm, digits, ndigits, state->radix, state->negativep ? -1 : 1);
  else
  {
    scm_int val = (scm_int)uval;
    state->n.setint(state->negativep ? -val : val);
  }
  return true;
}

static bool scm_readurealde(VM* vm, ReadNumState* state)
{
  switch(state->c) {
  case CASE_09DIGIT:case CASE_AFDIGIT:
    return scm_readuinteger(vm, state);
  default: return false;
  }
}

static bool scm_readdecimalfromdot(VM* vm, ReadNumState* state, scm_float beforedot)
{
  if (state->radix != 10) return false;
  scm_float de = state->radix;
  scm_float val = beforedot;
  if (std::isdigit(state->c))
  {
    do {
      int digit = digit_value(state->c);
      val = val + digit/de;
      state->next();
      de*=state->radix;
    } while (std::isdigit(state->c));
  }
  if (state->c == '#')
    do { state->next(); } while(state->c == '#');
  switch(state->c) {
  case CASE_EXPMARK:
    if (state->radix != 10) return false;
    state->next();
    return scm_readsuffix(vm, state, val);
  case '+':case '-':case '@':case 'i':case -1:
    state->n.setreal(val);
    return true;
  default:
    return false;
  }
}

static bool scm_readureal(VM* vm, ReadNumState* state)
{
  switch(state->c) {
  case '.':{
    state->next();
    switch(state->c) {
    case CASE_09DIGIT:
    case CASE_AFDIGIT: {
      if (!scm_readdecimalfromdot(vm, state, 0)) return false;
      if (state->negativep && state->n.type == NREAL)
        state->n.num.real = -state->n.num.real;
      return true;
    }
    default: return false;
    }
  }
  case CASE_09DIGIT:case CASE_AFDIGIT: {
    if (!scm_readuinteger(vm, state)) return false;
    switch(state->c) {
    case '/': {
      state->next();
      scm_int nu = state->n.num.inum;
      state->negativep = false;
      if (!scm_readurealde(vm, state)) return false;
      state->n.setratio(nu, state->n.num.inum);
      return true;
    }
    case '.':
      if (!state->next()) return false;
      {
        scm_int beforedot = state->negativep ? -state->n.num.inum
                                             : state->n.num.inum;
        if (!scm_readdecimalfromdot(vm, state, beforedot)) return false;
        if (state->negativep && state->n.type == NREAL)
          state->n.num.real = -state->n.num.real;
        return true;
      }
    case CASE_EXPMARK: {
      if (state->radix != 10) return false;
      state->next();
      return scm_readsuffix(vm, state, state->n.num.inum);
    }
    case '+':case '-':case CASE_I:case -1: return true;
    default: return false;
    }
  }
  default: return false;
  }
}

static bool scm_makenumtovt(VM* vm, ReadNumState* state, ValueT* out)
{
  if (state->bigval) {
    bigsetvt(vm, out, state->bigval);
    return true;
  }
  switch(state->n.type) {
  case NINT:
    setnumi(out, state->n.num.inum); break;
  case NREAL:
    setnumreal(out, state->n.num.real); break;
  case NRATIO:
    setnumratio(out, Sr2(vm, NumRatioObj, state->n.num.ratio.nu, state->n.num.ratio.de));
    break;
  default: return false;
  }
  return true;
}

static bool scm_makeregularreal1i(VM* vm, ReadNumState* state, ValueT* out)
{
  scm_float real = 0;
  switch(state->n.type) {
  case NINT:
    real = state->n.num.inum;
    break;
  case NREAL:
    real = state->n.num.real;
    break;
  case NRATIO:
    real = state->n.num.ratio.nu / (scm_float) state->n.num.ratio.de;
    break;
  default: return false;
  }
  setnumcomplex(out, Sr2(vm, NumComplexObj, real, state->negativep ? -1 : 1));
  return true;
}

static bool scm_readreal(VM* vm, ReadNumState* state)
{
  state->negativep = false;
  switch(state->c) {
  case '-':
    state->negativep = true;
  case '+':
    state->next();
    break;
  }
  return scm_readureal(vm, state);
}

static bool scm_makepolar(VM* vm, ReadNumState* state, VCoNum mag, VCoNum theta, ValueT* out)
{
  scm_float fmega = 0;
  switch(mag.type) {
  case NINT: fmega = mag.num.inum;break;
  case NREAL: fmega = mag.num.real;break;
  case NRATIO:
    fmega = mag.num.ratio.nu / (scm_float)mag.num.ratio.de;
    break;
  default: return false;
  }
  scm_float ftheta = 0;
  switch(theta.type) {
  case NINT: ftheta = theta.num.inum;break;
  case NREAL: ftheta = theta.num.real;break;
  case NRATIO:
    ftheta = theta.num.ratio.nu / (scm_float)theta.num.ratio.de;
    break;
  default: return false;
  }
  scm_float real, imag;
  SCMMath::complexpolar2rectreal(fmega, ftheta, &real);
  SCMMath::complexpolar2rectimag(fmega, ftheta, &imag);
  setnumcomplex(out, Sr2(vm, NumComplexObj, real, imag));
  return true;
}

static bool scm_makeregular(VM* vm, ReadNumState* state, VCoNum real, VCoNum imag, ValueT* out)
{
  scm_float freal, fimag;
  switch(real.type) {
  case NINT: freal = real.num.inum; break;
  case NREAL: freal = real.num.real; break;
  case NRATIO: freal = real.num.ratio.nu / (scm_float) real.num.ratio.de; break;
  default: return false;
  }
  switch(imag.type) {
  case NINT: fimag = imag.num.inum; break;
  case NREAL: fimag = imag.num.real; break;
  case NRATIO: fimag = imag.num.ratio.nu / (scm_float) imag.num.ratio.de; break;
  default: return false;
  }
  setnumcomplex(out, Sr2(vm, NumComplexObj, freal, fimag));
  return true;
}

static bool scm_readcomplexrealimag(VM* vm, ReadNumState* state, ValueT* out)
{
  if (!scm_readureal(vm, state)) return false;
  if (state->c == -1) return scm_makenumtovt(vm, state, out);
  VCoNum vn = state->n;
  switch(state->c) {
  case '@': {
    state->next();
    return scm_readreal(vm, state) &&
      state->c == -1 &&
      scm_makepolar(vm, state, vn, state->n, out);
  }
  case '-':
    state->negativep = true;
  case '+': {
    state->next();
    if (state->c == 'i')
    {
      state->next();
      return state->c == -1 && scm_makeregularreal1i(vm, state, out);
    }
    break;
  }
  default: return false;
  }
  if (!scm_readureal(vm, state)) return false;
  if (state->c != 'i') return false;
  state->next();
  return state->c == -1 && scm_makeregular(vm, state, vn, state->n, out);
}

static bool scm_readcomplex(VM* vm, ReadNumState* state, ValueT* out)
{
  switch(state->c) {
  case '-': state->negativep = true;
  case '+': {
    state->next();
    switch(state->c) {
    case 'i':case 'I': {
      state->next();
      if (state->c == -1)
      {
        setnumcomplex(out, Sr2(vm, NumComplexObj, 0, state->negativep ? -1 : 1));
        return true;
      }
      if (state->c == 'n' || state->c == 'N')
      {
        if (state->next() && (state->c == 'f' || state->c == 'F'))
          if (state->next() && state->c == '.')
            if (state->next() && state->c == '0')
            {
              state->next();
              if (state->c == -1)
              {
                scm_float real = state->negativep ? -INFINITY: INFINITY;
                setnumreal(out, real);
                return true;
              }
            }
      }
      return false;
    }
    case 'n':case 'N': {
      if (state->next() && (state->c == 'A' || state->c == 'a'))
      {
        if (state->next() && (state->c == 'N' || state->c == 'n'))
          if (state->next() && state->c == '.')
            if (state->next() && state->c == '0')
            {
              state->next();
              if (state->c == -1)
              {
                setnumreal(out, NAN);
                return true;
              }
            }
      }
      return false;
    }
    }
    break;
  }
  }
  return scm_readcomplexrealimag(vm, state, out);
}

static bool scm_readnum(VM* vm, ReadNumState* state, ValueT* out)
{
  if (state->c == '#') {
    state->next();
    bool e = false;
    switch(state->c) {
    case 'b':case 'B':
      state->radix=2; state->next(); break;
    case 'o':case 'O':
      state->radix=8; state->next(); break;
    case 'd':case 'D':
      state->radix=10; state->next(); break;
    case 'x':case 'X':
      state->radix=16; state->next(); break;
    case 'i':case 'I':
      state->exactp=false;e=true;state->next(); break;
    case 'e':case 'E':
      state->exactp=true;e=true; state->next(); break;
    default: return false;
    }
    if (state->c == '#')
    {
      state->next();
      switch(state->c) {
      case 'b':case 'B': if (!e) return false;
        state->radix=2; state->next(); break;
      case 'o':case 'O': if (!e) return false;
        state->radix=8; state->next(); break;
      case 'd':case 'D': if (!e) return false;
        state->radix=10; state->next(); break;
      case 'x':case 'X': if (!e) return false;
        state->radix=16; state->next(); break;
      case 'i':case 'I': if (e) return false;
        state->exactp=false; state->next(); break;
      case 'e':case 'E': if (e) return false;
        state->exactp=true; state->next(); break;
      default: return false;
      }
    }
  }
  return scm_readcomplex(vm, state, out);
}

static ValueT scm_stub_string2number(VM* vm, ValueT* z, ValueT* r)
{
  static const char* METHOD = "string->number";
  AssertVT(vm, isstr(z), z, "%s: not a string", METHOD);
  int len = vtstrlen(z);
  if (len <= 0) return Sfalseref;
  int radix = 10;
  if (!isnull(r))
  {
    ValueT* ra = Scar(r);
    AssertVT(vm, isnumi(ra), ra, "%s: not an integer", METHOD);
    radix = numi(ra);
    AssertVT(vm, radix == 2 || radix == 8 || radix == 10 || radix == 16,
             ra, "%s: not an integer", METHOD);
  }
  Sgcvar1(vm, out);
  const char* cstr = vtstr(z);
  ReadNumState state;
  state.exactp = true;
  state.negativep = false;
  state.radix = radix;
  state.zp = cstr;
  state.zn = len;
  state.bigval = NULL;
  state.next();
  if (!scm_readnum(vm, &state, out)) return Sfalseref;
  return out;
}

static void scm_uint2str(Lbuffer* numbuf, scm_int num, int radix)
{
  if (num < 0)
  {
    numbuf->put('-');
    num = -num;
  }
  if (num > 0)
  {
    int start = numbuf->count;
    do {
    int d = num % radix;
    num /= radix;
    numbuf->put(number_chars[d]);
    } while(num > 0);
    numbuf->reverse(start);
  }
  else
    numbuf->put('0');
}

static void scm_real2str(Lbuffer* numbuf, scm_float num)
{
  if (std::isinf(num))
    numbuf->put(num > 0 ? "+inf.0":"-inf.0");
  else if (std::isnan(num))
    numbuf->put("+nan.0");
  else if (num == 0.0)
    numbuf->put(signbit(num) != 0 ? "-0.0" : "0.0");
  else
  {
    if (num < 0)
    {
      numbuf->put('-');
      num = -num;
    }
    char buf[20] = {0};
    int d = num;
    num -= d;
    int i = snprintf(buf, sizeof(buf), "%u", d);
    buf[i++] = '.';
    numbuf->put(buf, i);

    if (!SCMMath::isFloatEqual(num, 0.0))
    {
      do {
        num *= 10;
        d = num;
        num -= d;
        i = snprintf(buf, sizeof(buf), "%u", d);
        buf[i] = 0;
        numbuf->put(buf, i);
      } while (!SCMMath::isFloatEqual(num, 0.0));
    }
    else
      numbuf->put('0');
  }
}

static ValueT scm_stub_number2string(VM* vm, ValueT* z, ValueT* r)
{
  static const char* METHOD = "number->string";
  AssertVT(vm, isnumber(z), z, "%s: not a number", METHOD);
  int radix = 10;
  if (!isnull(r))
  {
    ValueT* ra = Scar(r);
    AssertVT(vm, isnumi(ra), ra, "%s: not an integer", METHOD);
    radix = numi(ra);
    AssertVT(vm, radix == 2 || radix == 8 || radix == 10 || radix == 16,
             ra, "%s: not an integer", METHOD);
  }
  Lbuffer numbuf(vm);
  if (isnumi(z))
    scm_uint2str(&numbuf, numi(z), radix);
  else if (isnumreal(z))
  {
    Assert(vm, radix == 10, "%s: inexact numbers can only be printed in base 10", METHOD);
    scm_real2str(&numbuf, numreal(z));
  }
  else if (isnumratio(z))
  {
    scm_uint2str(&numbuf, numrationu(z), radix);
    numbuf.put('/');
    scm_uint2str(&numbuf, numratiode(z), radix);
  }
  else if (isnumbig(z))
  {
    Assert(vm, radix == 10, "%s: big numbers only support in base 10", METHOD);
    numbigref(z)->tostr(vm, &numbuf);
  }
  else
  {
    Assert(vm, radix == 10, "%s: complex real and imag part numbers can only be printed in base 10", METHOD);
    scm_real2str(&numbuf, numcomplexreal(z));
    scm_float imag = numcompleximag(z);
    if (imag > 0.0 && isnormal(imag))
      numbuf.put('+');
    scm_real2str(&numbuf, imag);
    numbuf.put('i');
  }
  ValueT out;
  setstr(&out, vm->strintern(numbuf.buf, numbuf.count));
  return out;
}

static ValueT scm_stub_make_polar(VM* vm, ValueT* x, ValueT* y)
{
  static const char* METHOD_NAME = "make-polar";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  AssertArg(vm, isnumber(y), METHOD_NAME, y, "not a number");

  scm_float xr = 0, yr = 0;
  if (!SCMMath::toreal(x, &xr))
    ErrorVT(vm, x, "cannot convert to real");
  if (!SCMMath::toreal(y, &yr))
    ErrorVT(vm, y, "cannot convert to real");

  ValueT out;
  scm_float real, imag;
  SCMMath::complexpolar2rectreal(xr, yr, &real);
  SCMMath::complexpolar2rectimag(xr, yr, &imag);
  setnumcomplex(&out, Sr2(vm, NumComplexObj, real, imag));
  return out;
}

static ValueT scm_stub_magnitude(VM* vm, ValueT* x)
{
  static const char* METHOD_NAME = "magnitude";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  ValueT out;
  if (isnumcomplex(x))
  {
    double xi = 0;
    SCMMath::complexrect2polarmag(numcomplexreal(x), numcompleximag(x), &xi);
    setnumreal(&out, xi);
  }
  else
    ErrorVT(vm, x, "%s:not a complex number", METHOD_NAME);
  return out;
}

static ValueT scm_stub_angle(VM* vm, ValueT* x)
{
  static const char* METHOD_NAME = "angle";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  ValueT out;
  if (isnumcomplex(x))
  {
    double xi = 0;
    SCMMath::complexrect2polarangle(numcomplexreal(x), numcompleximag(x), &xi);
    setnumreal(&out, xi);
  }
  else
    ErrorVT(vm, x, "%s:not a complex number", METHOD_NAME);
  return out;
}

static ValueT scm_stub_imag_part(VM* vm, ValueT* x)
{
  static const char* METHOD_NAME = "imag-part";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  ValueT out;
  if (isnumcomplex(x))
    setnumreal(&out, numcompleximag(x));
  else
    ErrorVT(vm, x, "%s:not a complex number", METHOD_NAME);

  return out;
}

static ValueT scm_stub_real_part(VM* vm, ValueT* x)
{
  static const char* METHOD_NAME = "real-part";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  ValueT out;
  if (isnumcomplex(x))
    setnumreal(&out, numcomplexreal(x));
  else
    ErrorVT(vm, x, "%s: not a complex number", METHOD_NAME);
  return out;
}

static ValueT scm_stub_make_rectangular(VM* vm, ValueT* x, ValueT* y)
{
  static const char* METHOD_NAME = "make-rectangular";
  AssertArg(vm, isnumber(x), METHOD_NAME, x, "not a number");
  AssertArg(vm, isnumber(y), METHOD_NAME, y, "not a number");

  double xr = 0, yr = 0;
  if (!SCMMath::toreal(x, &xr))
    ErrorVT(vm, x, "cannot convert to real");
  if (!SCMMath::toreal(y, &yr))
    ErrorVT(vm, y, "cannot convert to real");

  ValueT out;
  setnumcomplex(&out, Sr2(vm, NumComplexObj, xr, yr));
  return out;
}

static ValueT scm_stub_sqrt(VM* vm, ValueT* z)
{
  static const char* METHOD_NAME = "sqrt";
  AssertArg(vm, isnumber(z), METHOD_NAME, z, "not a number");
  ValueT out;
  if (isnumi(z))
  {
    double r = std::sqrt((double)numi(z));
    if (r == std::floor(r))
      setnumi(&out, (scm_int)r);
    else
      setnumreal(&out, r);
  }
  else if (isnumreal(z))
    setnumreal(&out, std::sqrt(numreal(z)));
  else if (isnumratio(z))
    setnumreal(&out, std::sqrt(numrationu(z) / (double)numratiode(z)));
  else
    ErrorVT(vm, z, "%s: parameter error", METHOD_NAME);
  return out;
}

static ValueT scm_stub_atan(VM* vm, ValueT* z, ValueT* y)
{
  static const char* MATH_NAME = "atan";
  AssertArg(vm, isnumber(z), MATH_NAME, z, "not a number");
  ValueT out;
  if (isnull(y))
  {
    double dz;
    if (isnumi(z))
      dz = numi(z);
    else if (isnumreal(z))
      dz = numreal(z);
    else if (isnumratio(z))
      dz = numrationu(z) / (double)numratiode(z);
    else
      ErrorVT(vm, z, "%s: parameter error", MATH_NAME);

    setnumreal(&out, atan (dz));
  }
  else
  {
    AssertArg(vm, isnull(Scdr(y)), MATH_NAME, y, "too many paramters");
    y = Scar(y);
    AssertArg(vm, isnumber(y), MATH_NAME, y, "not a number");

    double dz;
    if (isnumi(z))
      dz = numi(z);
    else if (isnumreal(z))
      dz = numreal(z);
    else if (isnumratio(z))
      dz = numrationu(z) / (double)numratiode(z);
    else
      ErrorVT(vm, z, "%s: parameter error", MATH_NAME);

    double dy;
    if (isnumi(y))
      dy = numi(y);
    else if (isnumreal(y))
      dy = numreal(y);
    else if (isnumratio(y))
      dy = numrationu(y) / (double)numratiode(y);
    else
      ErrorVT(vm, y, "%s: parameter error", MATH_NAME);

    setnumreal(&out, atan2 (dz, dy));
  }
  return out;
}

static ValueT scm_stub_floor(VM* vm, ValueT* p)
{
  static const char* METHOD = "floor";
  if (isnumi(p))
    return p;
  else if (isnumreal(p))
  {
    ValueT out;
    setnumreal(&out, floor(numreal(p)));
    return out;
  }
  else if (isnumratio(p))
  {
    scm_int nu = numrationu(p), de = numratiode(p);
    scm_int q = nu / de;
    scm_int r = nu % de;
    if (r != 0 && ((nu ^ de) < 0))
      q -= 1;
    ValueT out;
    setnumi(&out, q);
    return out;
  }
  else
    ErrorVT(vm, p, "%s: parameter error", METHOD);
  return Sundefined;
}

static ValueT scm_stub_ceiling(VM* vm, ValueT* p)
{
  static const char* METHOD = "ceiling";
  if (isnumi(p))
    return p;
  else if (isnumreal(p))
  {
    ValueT out;
    setnumreal(&out, ceil(numreal(p)));
    return out;
  }
  else if (isnumratio(p))
  {
    scm_int nu = -numrationu(p), de = numratiode(p);
    scm_int q = nu / de;
    scm_int r = nu % de;
    if (r != 0 && ((nu ^ de) < 0))
      q -= 1;
    ValueT out;
    setnumi(&out, -q);
    return out;
  }
  else
    ErrorVT(vm, p, "%s: parameter error", METHOD);
  return Sundefined;
}

static ValueT scm_stub_truncate(VM* vm, ValueT* p)
{
  static const char* METHOD = "truncate";
  if (isnumi(p))
    return p;
  else if (isnumreal(p))
  {
    ValueT out;
    setnumreal(&out, ceil(numreal(p)));
    return out;
  }
  else if (isnumratio(p))
  {
    scm_int nu = numrationu(p), de = numratiode(p);
    ValueT out;
    setnumi(&out, (nu / de));
    return out;
  }
  else
    ErrorVT(vm, p, "%s: parameter error", METHOD);
  return Sundefined;
}

static scm_int exact_round(scm_int nu, scm_int de)
{
  scm_int q = nu / de;
  scm_int r = nu % de;
  if (r != 0)
  {
    bool neg = ((nu < 0) != (de < 0));
    scm_int ar = r < 0 ? -r : r;    // |r|
    scm_int ad = de < 0 ? -de : de; // |de|
    if (ar + ar > ad)
      return neg ? q - 1 : q + 1;
    if (ar + ar < ad)
      return q;
    if (q % 2 != 0)
      return neg ? q - 1 : q + 1;
  }
  return q;
}

static double roundhalf2even(double x)
{
  double t = trunc(x);
  double frac = x - t;
  if (frac > 0.5 || frac < -0.5)
    return x > 0 ? t + 1 : t - 1;
  if ((frac == 0.5 || frac == -0.5) && fmod(t, 2.0) != 0.0)
    return x > 0 ? t + 1 : t - 1;
  return t;
}

static ValueT scm_stub_round(VM* vm, ValueT* p)
{
  static const char* METHOD = "round";
  if (isnumi(p))
    return p;
  else if (isnumreal(p))
  {
    ValueT out;
    setnumreal(&out, roundhalf2even(numreal(p)));
    return out;
  }
  else if (isnumratio(p))
  {
    ValueT out;
    setnumi(&out, exact_round(numrationu(p), numratiode(p)));
    return out;
  }
  else
    ErrorVT(vm, p, "%s: parameter error", METHOD);
  return Sundefined;
}

static ValueT scm_stub_exact2inexact(VM* vm, ValueT* z)
{
  static const char* METHOD_NAME = "exact->inexact";
  ValueT out;
  if (isnumi(z))
    setnumreal(&out, numi(z));

  else if (isnumreal(z))
    out = z;

  else if (isnumratio(z))
  {
    double nu = numrationu(z);
    double de = numratiode(z);
    setnumreal(&out, nu / de);
  }

  else if (isnumcomplex(z))
    out = z;

  else if (isnumbig(z))
    setnumreal(&out, big2double(numbigref(z)));

  else
    ErrorVT(vm, z, "%s: error parameter", METHOD_NAME);
  return out;
}

static ValueT scm_stub_inexact2exact(VM* vm, ValueT* z)
{
  static const char* METHOD_NAME = "inexact->exact";
  ValueT out;
  if (SCMMath::exactp(z))
    out = z;

  else if (isnumreal(z))
  {
    scm_float zr = numreal(z);
    AssertVT(vm, !isinf(zr) && !isnan(zr), z, "%s:  not a finite number", METHOD_NAME);
    scm_int nu = 0, de = 1;
    scm_float _;
    SCMMath::float2ratio(zr, &_, &nu, &de, 10, 0);

    if (de == 1)
      setnumi(&out, nu);

    else
      setnumratio(&out, Sr2(vm, NumRatioObj, nu, de));
  }

  else
    ErrorVT(vm, z, "%s: not supported yet", METHOD_NAME);
  return out;
}

static ValueT scm_stub_inexactp(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "inexact?";
  AssertArg(vm, isnumber(p), METHOD_NAME, p, " not a number");
  return frombool(!SCMMath::exactp(p));
}

static ValueT scm_stub_abs(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "abs";
  AssertArg(vm, isnumber(p), METHOD_NAME, p, " not a number");
  ValueT p2;
  setnumi(&p2, 0);
  Sgcvar2(vm, np, np2);
  numsynctype(vm, np, np2, p, &p2);
  if (SCMMath::numlessp(np, np2))
  {
    ValueT o2;
    numsubtract(vm, &o2, np2, np);
    return o2;
  }
  else
    return *np;
}

static ValueT scm_stub_negativep(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "negative?";
  AssertArg(vm, isnumber(p), METHOD_NAME, p, " not a number");

  ValueT p2;
  setnumi(&p2, 0);

  Sgcvar2(vm, np, np2);
  numsynctype(vm, np, np2, p, &p2);
  return frombool(SCMMath::numlessp(np, np2));
}

static ValueT scm_stub_equal(VM* vm, ValueT* p1, ValueT* args)
{
  static const char* METHOD_NAME = "=";
  AssertArg(vm, isnumber(p1), METHOD_NAME, p1, " not a number");
  ValueT* n = p1;
  Sgcvar2(vm, np, n1p);
  PAIR_FOR(p, args)
  {
    ValueT* n1 = Scar(p);
    AssertArg(vm, isnumber(n1), METHOD_NAME, n1, " not a number");
    numsynctype(vm, np, n1p, n, n1);
    n = np, n1 = n1p;
    if (!SCMMath::numequalp(n, n1))
      return Sfalseref;
    *n = n1;
  }
  return Strueref;
}

static ValueT scm_stub_numberp(VM* vm, ValueT* p)
{
  return frombool(isnumber(p));
}

static ValueT scm_stub_complexp(VM* vm, ValueT* p)
{
  return frombool(isnumcomplex(p) || isnumreal(p) || isnumratio(p) || isnumi(p) || isnumbig(p));
}

static ValueT scm_stub_realp(VM* vm, ValueT* p)
{
  return frombool(isnumreal(p) || isnumratio(p) || isnumi(p) || isnumbig(p));
}

static ValueT scm_stub_rationalp(VM* vm, ValueT* p)
{
  return frombool(isnumratio(p) || isnumi(p) || isnumbig(p));
}

static ValueT scm_stub_integerp(VM* vm, ValueT* p)
{
  return frombool(isnumi(p) || isnumbig(p));
}

static ValueT scm_stub_zerop(VM* vm, ValueT* p)
{
  static const char* METHOD = "zero?";
  AssertVT(vm, isnumber(p), p,"%s: not a number", METHOD);
  return frombool(SCMMath::isnumzero(vm, p));
}

static ValueT scm_stub_evenp(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "even?";
  if (isnumi(p))
    return frombool(numi(p) % 2 == 0);
  if (isnumbig(p))
    return frombool((numbigref(p)->data[0] & 1) == 0);
  AssertArg(vm, false, METHOD_NAME, p, " not an integer");
  return Sfalseref;
}

static ValueT scm_stub_oddp(VM* vm, ValueT* p)
{
  static const char* METHOD_NAME = "odd?";
  if (isnumi(p))
    return frombool(numi(p) % 2 != 0);
  if (isnumbig(p))
    return frombool((numbigref(p)->data[0] & 1) != 0);
  AssertArg(vm, false, METHOD_NAME, p, " not an integer");
  return Sfalseref;
}

void SCMMath::init(VM* vm)
{
  const RegCProc mathlib[] = {
    {"+", scm_stub_add, true},
    {"-", scm_stub_subtract, true},
    {"*", scm_stub_multiply, true},
    {"/", scm_stub_divide, true},
    {"=", scm_stub_equal, true},
    {">", scm_stub_bigger, true},
    {">=", scm_stub_biggereq, true},
    {"<", scm_stub_less, true},
    {"<=", scm_stub_lesseq, true},

    {"quotient", scm_stub_quotient},
    {"remainder", scm_stub_remainder},
    {"modulo", scm_stub_modulo},
    {"abs", scm_stub_abs},
    {"max", scm_stub_max, true},
    {"min", scm_stub_min, true},
    {"gcd", scm_stub_gcd, true},
    {"lcm", scm_stub_lcm, true},
    {"numerator", scm_stub_numerator},
    {"denominator", scm_stub_denominator},

    {"exact->inexact", scm_stub_exact2inexact},
    {"inexact->exact", scm_stub_inexact2exact},

    STUB_REG1(floor),
    STUB_REG1(ceiling),
    STUB_REG1(truncate),
    STUB_REG1(round),

    STUB_REG1(exp),
    STUB_REG1(log),
    STUB_REG1(sin),
    STUB_REG1(cos),
    STUB_REG1(tan),
    STUB_REG1(asin),
    STUB_REG1(acos),
    {"atan", scm_stub_atan, true},

    STUB_REG1(sqrt),
    STUB_REG1(expt),

    {"make-rectangular", scm_stub_make_rectangular},
    {"make-polar", scm_stub_make_polar},
    {"real-part", scm_stub_real_part},
    {"imag-part", scm_stub_imag_part},
    {"magnitude", scm_stub_magnitude},
    {"angle", scm_stub_angle},

    {"number->string", scm_stub_number2string, true},
    {"string->number", scm_stub_string2number, true},

    {"rationalize", scm_stub_rationalize},

    {"number?", scm_stub_numberp},
    {"complex?", scm_stub_complexp},
    {"real?", scm_stub_realp},
    {"rational?", scm_stub_rationalp},
    {"integer?", scm_stub_integerp},
    {"negative?", scm_stub_negativep},
    {"positive?", scm_stub_positivep},
    {"exact?", scm_stub_exactp},
    {"inexact?", scm_stub_inexactp},
    {"zero?", scm_stub_zerop},
    {"even?", scm_stub_evenp},
    {"odd?", scm_stub_oddp},
    {NULL, -1}
  };
  regcfunc(vm, mathlib);
}
};

#pragma once

#include "dummy.h"
#include "value.h"
#include "reader.h"

namespace Dummy{

#define Snil (Scheme::Nil);
#define Snull (Scheme::Null);
#define Strue (Scheme::True);
#define Sfalse (Scheme::False);

#define Squote Scheme::Quote;
#define Sunquote Scheme::UnQuote;
#define Sunquotesplicing Scheme::UnQuoteSplicing;
#define Squasiquote Scheme::QuasiQuote;

#define Sset_ Scheme::SetMark;
#define Sdefine Scheme::Define;
#define Slambda Scheme::Lambda;
#define Sif Scheme::If;
#define Sprocedure Scheme::Procedure;
#define Sbegin Scheme::Begin;
#define Scond Scheme::Cond;
#define Selse Scheme::Else;

#define Snullp Scheme::nullp
#define Struep Scheme::truep

#define Splus Scheme::plus
#define Scons Scheme::cons
#define Slist Scheme::list
#define Scar Scheme::car
#define Scdr Scheme::cdr
#define Scadr Scheme::cadr
#define Scddr Scheme::cddr
#define Scaadr Scheme::caadr
#define Scaddr Scheme::caddr
#define Scdadr Scheme::cdadr
#define Scdddr Scheme::cdddr
#define Scadddr Scheme::cadddr

};

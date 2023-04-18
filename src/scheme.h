#pragma once

#include "dummy.h"
#include "value.h"
#include "reader.h"

namespace Dummy{

#define Svoid Scheme::Void
#define Snil Scheme::Nil
#define Snull Scheme::Null
#define Strue Scheme::True
#define Sfalse Scheme::False

#define Squote Scheme::quote
#define Sunquote Scheme::unquote
#define Sunquotesplicing Scheme::unquotesplicing
#define Squasiquote Scheme::quasiquote

#define Spairp Scheme::pairp
#define Snullp Scheme::nullp
#define Struep Scheme::truep
#define Ssymbolp Scheme::symbolp

#define Sprimitivep Scheme::primitivep

#define Slength Scheme::length
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

#define Sreverse Scheme::reverse
#define Smake_lambda Scheme::make_lambda

};

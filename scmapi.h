#pragma once
// =====================================================================
// scmapi.h — Scheme extension (stub) API
//
// Design principles:
//   · Strict typing: no implicit conversion. 2.0 is NOT treated as 2.
//   · No type enum exposed; integers/floats use vm.h's scm_int/scm_float,
//     chars use scm_char (= uchar).
//   · Rest parameters reuse the pair list VM already packs — no array.
//   · Errors print only "expected <type>, got <value>".
//   · Registration reuses the existing mechanism: RegCProc[] + regcfunc.
//
//
// ── 1. Function signatures ─────────────────────────────
//   fixed N args: static ValueT fn(VM* vm, ValueT* p1, ..., ValueT* pN);
//   rest:         static ValueT fn(VM* vm, ValueT* rest);
//                 static ValueT fn(VM* vm, ValueT* p1, ValueT* rest);
//                 (rest IS the pair list VM packed; just traverse it)
//
// ── 2. Registration (same as builtin libs, no new macros) ─
//   static const RegCProc myext[] = {
//     RegCProc("my-add", scm_stub_my_add),        // CProc2 → fixed 2 args
//     RegCProc("my-sum", scm_stub_my_sum, true),  // true   → rest (pair list)
//     RegCProc(NULL, -1)
//   };
//   regcfunc(vm, myext);      // identical to hashes[] in scmtable.cpp
//
// ── 3. Type API ─────────────────────────────────────────
//   if (scm_is_string(vm, p)) { ... }              // branch by type
//   scm_int x = scm_get_int(vm, p);                // strict: integer only
//   scm_float f = scm_get_float(vm, p);            // strict: float only
//   ValueT r = scm_make_int(vm, x);                // build; return it
//   return_void();                                 // return void
// =====================================================================

#if defined(_WIN32) && defined(SCMAPI_EXPORTS)
#define SCM_API extern __declspec(dllexport)
#elif defined(_WIN32)
#define SCM_API extern __declspec(dllimport)
#else
#define SCM_API extern
#endif

namespace Scheme {

// ---------- type predicates (dispatch branches by arg type) ----------
SCM_API bool scm_is_int(VM* vm, ValueT* p);      // plain integer only (big not exposed yet)
SCM_API bool scm_is_float(VM* vm, ValueT* p);
SCM_API bool scm_is_string(VM* vm, ValueT* p);
SCM_API bool scm_is_bool(VM* vm, ValueT* p);
SCM_API bool scm_is_true(VM* vm, ValueT* p);     // the #t value only
SCM_API bool scm_is_false(VM* vm, ValueT* p);    // the #f value only
SCM_API bool scm_is_char(VM* vm, ValueT* p);
SCM_API bool scm_is_pair(VM* vm, ValueT* p);     // non-empty list
SCM_API bool scm_is_list(VM* vm, ValueT* p);     // pair or '()
SCM_API bool scm_is_null(VM* vm, ValueT* p);
SCM_API bool scm_is_vector(VM* vm, ValueT* p);
SCM_API bool scm_is_table(VM* vm, ValueT* p);

// ---------- getters (strict; error: expected <type>, got <value>) ----------
SCM_API scm_int     scm_get_int(VM* vm, ValueT* p);
SCM_API scm_float   scm_get_float(VM* vm, ValueT* p);
SCM_API const char* scm_get_string(VM* vm, ValueT* p, int* len);
SCM_API bool        scm_get_bool(VM* vm, ValueT* p);
SCM_API scm_char    scm_get_char(VM* vm, ValueT* p);

// ---------- list access (pair list; all return ValueT* slots) ----------
SCM_API ValueT* scm_car(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdr(VM* vm, ValueT* p);
SCM_API ValueT* scm_caar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_caaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_caadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cadar(VM* vm, ValueT* p);
SCM_API ValueT* scm_caddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cddar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_caaaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_caaadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_caadar(VM* vm, ValueT* p);
SCM_API ValueT* scm_caaddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cadaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cadadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_caddar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cadddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdaaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdaadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdadar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdaddr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cddaar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cddadr(VM* vm, ValueT* p);
SCM_API ValueT* scm_cdddar(VM* vm, ValueT* p);
SCM_API ValueT* scm_cddddr(VM* vm, ValueT* p);
SCM_API int     scm_list_len(VM* vm, ValueT* list);
SCM_API ValueT* scm_list_ref(VM* vm, ValueT* list, int i);   // i-th element slot (0-based)

// ---------- constructors (return ValueT) ----------
SCM_API ValueT scm_make_int(VM* vm, scm_int x);
SCM_API ValueT scm_make_float(VM* vm, scm_float x);
SCM_API ValueT scm_make_bool(VM* vm, bool b);
SCM_API ValueT scm_make_char(VM* vm, scm_char c);
SCM_API ValueT scm_make_string(VM* vm, const char* s);             // len = strlen(s)
SCM_API ValueT scm_make_string(VM* vm, const char* s, int len);    // explicit len
SCM_API ValueT scm_make_vector(VM* vm, int n);
SCM_API ValueT scm_make_list(VM* vm, ValueT* items, int n);
SCM_API ValueT scm_make_pair(VM* vm, ValueT* a, ValueT* b);
SCM_API ValueT scm_make_table(VM* vm);
SCM_API void   scm_vector_set(VM* vm, ValueT* vec, int i, ValueT v);
SCM_API ValueT scm_vector_ref(VM* vm, ValueT* vec, int i);
SCM_API void   scm_table_set(VM* vm, ValueT* tbl, ValueT* key, ValueT val);
SCM_API ValueT scm_table_ref(VM* vm, ValueT* tbl, ValueT* key);

// ---------- error (throws Scheme exception after printing fmt) ----------
SCM_API void scm_error(VM* vm, const char* fmt, ...);

// ---------- void return macro (contains return; just write return_void();) ----------
#define return_void() return (*Svoidref)

} // namespace Scheme

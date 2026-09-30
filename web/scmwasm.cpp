// web/scmwasm.cpp — Emscripten entry point: drive the Scheme VM from JavaScript.
//
// Exported C ABI used by web/shell.html:
//   scm_wasm_init()        create the VM (runs the embedded init.scm)
//   scm_wasm_eval(src)     evaluate a source string, return malloc'd output
//   scm_wasm_failed()      1 if the last eval threw
//   scm_wasm_free(ptr)     free a string returned above
//   scm_wasm_version()     static version string
//
// Browser-only: lives under web/ and is built solely by web/build.sh, never by
// the native CMake build. The whole file is behind SCM_WASM so that compiling
// it elsewhere is a no-op.
#ifdef SCM_WASM

#include <emscripten/emscripten.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../vm.h"

using namespace Scheme;

namespace {

VM* g_vm = NULL;
bool g_failed = false;

// The VM reports values and errors through the C streams: `printvalue` writes
// to vm->oport (stderr by default) and the Assert/Error macros fprintf(stderr).
// Emscripten's stderr is the JS console, which the REPL UI cannot read, so we
// redirect it into a temp file and hand the text back to JS after each eval.
char g_outfile[] = "/tmp/scm-out.txt";
FILE* g_outstream = NULL;
long g_readpos = 0;   // bytes already handed back to JS

void ensure_redirect()
{
  if (g_outstream != NULL) return;
  // freopen reassigns the global `stderr` object itself, which is what the VM
  // captured when it built its default output port.
  g_outstream = freopen(g_outfile, "w+b", stderr);
}

// Return everything written since the last call.
std::string drain()
{
  if (g_outstream == NULL) return std::string();
  fflush(g_outstream);
  fseek(g_outstream, g_readpos, SEEK_SET);
  std::string out;
  char buff[4096];
  size_t n;
  while ((n = fread(buff, 1, sizeof(buff), g_outstream)) > 0)
    out.append(buff, n);
  g_readpos = ftell(g_outstream);
  return out;
}

char* dup_to_c(const std::string& s)
{
  char* ret = (char*)malloc(s.size() + 1);
  if (ret == NULL) return NULL;
  memcpy(ret, s.c_str(), s.size() + 1);
  return ret;
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void scm_wasm_init()
{
  ensure_redirect();
  if (g_vm != NULL) return;
  g_vm = new VM();
  drain();  // discard anything the bootstrap printed
}

// Non-zero when the last scm_wasm_eval ended in an exception. The thrown value
// is only the failed assert's condition text, which the VM already printed in a
// readable form, so the REPL just needs to know *that* it failed.
EMSCRIPTEN_KEEPALIVE int scm_wasm_failed()
{
  return g_failed ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE const char* scm_wasm_eval(const char* src)
{
  g_failed = false;
  scm_wasm_init();
  if (g_vm == NULL) { g_failed = true; return dup_to_c("cannot create VM\n"); }

  TRY {
    ValueT val = g_vm->evalstr(src);
    if (!isvoid(&val) && !isundefined(&val))
      g_vm->printvalue(&val);
  }
  CATCH(e) {
    g_failed = true;
  }
  catch(...) {
    g_failed = true;
  }

  std::string out = drain();
  if (out.empty() && g_failed) out = "unknown error\n";
  return dup_to_c(out);
}

EMSCRIPTEN_KEEPALIVE void scm_wasm_free(const char* ptr)
{
  if (ptr) free((void*)ptr);
}

EMSCRIPTEN_KEEPALIVE const char* scm_wasm_version()
{
  return "dummyscheme (wasm)";
}

} // extern "C"

int main()
{
  scm_wasm_init();
  return 0;
}

#endif // SCM_WASM

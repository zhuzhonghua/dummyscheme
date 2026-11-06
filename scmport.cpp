#include "vm.h"
#include "scmport.h"

namespace Scheme {

static OutputPortObj* getoportrest(VM* vm, ValueT* args, const char* METHOD)
{
  if (isnull(args))
    return NULL;
  ValueT* ovt = Scar(args);
  AssertVT(vm, isoport(ovt), ovt, "%s: not a output-port", METHOD);
  Assert(vm, isnull(Scdr(args)), "%s: too much arguments", METHOD);
  return oportref(ovt);
}

static InputPortObj* getiportrest(VM* vm, ValueT* args, const char* METHOD)
{
  if (isnull(args))
    return NULL;
  ValueT* ivt = Scar(args);
  AssertVT(vm, isiport(ivt), ivt, "%s: not a input-port", METHOD);
  Assert(vm, isnull(Scdr(args)), "%s: too much arguments", METHOD);
  return iportref(ivt);
}

static InputPortObj* getcuriport(VM* vm)
{
  CallFrame* fm = vm->curfrm;
  if (fm->env.curiport) return fm->env.curiport;
  return G(vm)->iport;
}

static OutputPortObj* getcuroport(VM* vm)
{
  CallFrame* fm = vm->curfrm;
  if (fm->env.curoport) return fm->env.curoport;
  return G(vm)->oport;
}

static ValueT scm_stub_current_iport(VM* vm)
{
  const static char* METHOD = "current-input-port";
  ValueT ret;
  setiport(&ret, getcuriport(vm));
  return ret;
}

static ValueT scm_stub_current_oport(VM* vm)
{
  const static char* METHOD = "current-output-port";
  ValueT ret;
  setoport(&ret, getcuroport(vm));
  return ret;
}

static ValueT scm_stub_write(VM* vm, ValueT* p, ValueT* args)
{
  const static char* METHOD = "write";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (oport)
    oport->write(vm, p);
  else
    getcuroport(vm)->write(vm, p);
  return Svoidref;
}

static ValueT scm_stub_newline(VM* vm, ValueT* args)
{
  const static char* METHOD = "newline";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (oport)
    oport->writechar('\n');
  else
    getcuroport(vm)->writechar('\n');
  return Svoidref;
}

static ValueT scm_stub_display(VM* vm, ValueT* p, ValueT* args)
{
  const static char* METHOD = "display";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (!oport)
    oport = getcuroport(vm);
  if (isstr(p))
  {
    StrPtr strp = strref(p);
    oport->writestr(Ssstr(strp), Sslen(strp));
  }
  else if (ischar(p))
  {
    char c = vtchar(p);
    oport->writechar(c);
  }
  else
    oport->write(vm, p);
  return Svoidref;
}

static ValueT scm_stub_load(VM* vm, ValueT* p)
{
  AssertArg(vm, isstr(p), "load", p, "not a string");
  vm->loadfile(Ssstr(strref(p)));

  return Svoidref;
}

/* -------------------------------------------------------------------------
 * require -- name-based module loading
 *
 *   (require "lib.util")
 *
 * The cache key is the module name exactly as passed to require.  The name
 * is not normalized and no path canonicalization is done.  The name is
 * mapped to a file through a template list (default
 * "./?.scm;./?/init.scm"):
 *
 *   '.' in the module name becomes the directory separator,
 *   '?' in each ';'-separated template is replaced by that name.
 *
 * The list is read from SCHEME_PATH once at startup, stored in the registry
 * and can be read or replaced at run time with (require-path) and
 * (require-path! "...").  A ";;" in a new list expands to the default list.
 * ---------------------------------------------------------------------- */

#ifndef SCM_PATH_MAX
#define SCM_PATH_MAX 4096
#endif

#if defined(_WIN32)
#define SCM_DIRSEP '\\'
#else
#define SCM_DIRSEP '/'
#endif

static const char* REQUIRE_CACHE_KEY = "@require.loaded";
static const char* REQUIRE_PATH_KEY = "@require.path";
static const char* REQUIRE_DEFAULT_PATH = "./?.scm;./?/init.scm";

static ValueT require_cache(VM* vm)
{
  Sgcvar1(vm, key);
  setstr(key, vm->strintern(REQUIRE_CACHE_KEY,
                            (int)strlen(REQUIRE_CACHE_KEY)));

  HashTableObj* reg = hashtableref(&G(vm)->registry);
  ValueT* slot = reg->get(key);
  if (slot && ishashtable(slot))
    return *slot;

  Sgcvar1(vm, tbl);
  sethashtable(tbl, Sr1(vm, HashTableObj, vm));
  reg->set(vm, key, tbl);
  return *tbl;
}

/* Build the path value, expanding the first ";;" to the default list. */
static ValueT require_make_path(VM* vm, const char* path)
{
  ValueT out;
  const char* mark = strstr(path, ";;");
  if (mark == NULL)
  {
    setstr(&out, vm->strintern(path, (int)strlen(path)));
    return out;
  }

  int plen = (int)(mark - path);
  const char* suffix = mark + 2;

  Lbuffer buf(vm);
  if (plen > 0)
  {
    buf.put(path, plen);
    buf.put(';');
  }
  buf.put(REQUIRE_DEFAULT_PATH);
  if (*suffix)
  {
    buf.put(';');
    buf.put(suffix);
  }

  setstr(&out, vm->strintern(buf.buf, buf.count));
  return out;
}

static ValueT require_set_path(VM* vm, const char* path)
{
  Sgcvar2(vm, key, val);
  setstr(key, vm->strintern(REQUIRE_PATH_KEY,
                            (int)strlen(REQUIRE_PATH_KEY)));
  *val = require_make_path(vm, path);
  hashtableref(&G(vm)->registry)->set(vm, key, val);
  return *val;
}

static ValueT require_get_path(VM* vm)
{
  Sgcvar1(vm, key);
  setstr(key, vm->strintern(REQUIRE_PATH_KEY,
                            (int)strlen(REQUIRE_PATH_KEY)));
  ValueT* slot = hashtableref(&G(vm)->registry)->get(key);
  if (slot && isstr(slot))
    return *slot;

  const char* path = getenv("SCHEME_PATH");
  if (path == NULL)
    path = REQUIRE_DEFAULT_PATH;
  return require_set_path(vm, path);
}

static bool require_readable(const char* path)
{
  FILE* f = fopen(path, "r");
  if (f == NULL) return false;
  fclose(f);
  return true;
}

/* Search the ';'-separated template list: replace '.' in the module name
 * with the directory separator, then substitute the name for '?' in each
 * template.  Return the first readable file. */
static bool require_searchpath(const char* name, const char* path,
                               char* out, int outlen)
{
  char namebuf[SCM_PATH_MAX];
  int n = 0;
  for (const char* p = name; *p && n < (int)sizeof(namebuf) - 1; ++p)
    namebuf[n++] = (*p == '.') ? SCM_DIRSEP : *p;
  namebuf[n] = '\0';

  const char* p = path;
  while (*p)
  {
    const char* end = strchr(p, ';');
    int plen = end ? (int)(end - p) : (int)strlen(p);

    int k = 0;
    for (int i = 0; i < plen && k < outlen - 1; ++i)
    {
      if (p[i] == '?')
      {
        for (int j = 0; namebuf[j] && k < outlen - 1; ++j)
          out[k++] = namebuf[j];
      }
      else
        out[k++] = p[i];
    }
    out[k] = '\0';

    if (require_readable(out))
      return true;

    if (!end) break;
    p = end + 1;
  }
  return false;
}

static ValueT scm_stub_require(VM* vm, ValueT* p)
{
  static const char* METHOD = "require";
  AssertArg(vm, isstr(p), METHOD, p, "not a string");

  const char* name = Ssstr(strref(p));

  Sgcvar3(vm, cache, key, pathvt);
  *cache = require_cache(vm);
  setstr(key, vm->strintern(name, (int)strlen(name)));

  HashTableObj* tab = hashtableref(cache);
  ValueT* hit = tab->get(key);
  if (hit)
  {
    if (isfalse(hit))
      Error(vm, "require: cyclic dependency while loading '%s'", name);
    return *hit;
  }

  *pathvt = require_get_path(vm);
  const char* path = Ssstr(strref(pathvt));
  char file[SCM_PATH_MAX];
  if (!require_searchpath(name, path, file, sizeof(file)))
    Error(vm, "require: module '%s' not found (require-path=%s)", name, path);

  tab->set(vm, key, Sfalseref);   /* loading */
  try
  {
    vm->loadfile(file);
  }
  catch (...)
  {
    tab->remove(vm, key);  /* do not cache failures */
    throw;
  }
  tab->set(vm, key, Strueref);    /* loaded */
  return Strueref;
}

/* (require-clear! "lib.util") -- forget a loaded module so that the next
 * require loads it again. */
static ValueT scm_stub_require_clear(VM* vm, ValueT* p)
{
  static const char* METHOD = "require-clear!";
  AssertArg(vm, isstr(p), METHOD, p, "not a string");

  const char* name = Ssstr(strref(p));

  Sgcvar2(vm, cache, key);
  *cache = require_cache(vm);
  setstr(key, vm->strintern(name, (int)strlen(name)));
  hashtableref(cache)->remove(vm, key);
  return Svoidref;
}

/* (require-path) -- current template list. */
static ValueT scm_stub_require_path(VM* vm)
{
  return require_get_path(vm);
}

/* (require-path! "templates") -- replace the template list. */
static ValueT scm_stub_require_path_set(VM* vm, ValueT* p)
{
  static const char* METHOD = "require-path!";
  AssertArg(vm, isstr(p), METHOD, p, "not a string");
  require_set_path(vm, Ssstr(strref(p)));
  return Svoidref;
}

static ValueT scm_stub_iportp(VM* vm, ValueT* p)
{
  return frombool(isiport(p));
}

static ValueT scm_stub_oportp(VM* vm, ValueT* p)
{
  return frombool(isoport(p));
}

static ValueT scm_stub_portp(VM* vm, ValueT* p)
{
  return frombool(isoport(p) || isiport(p));
}

static ValueT scm_stub_open_input_file(VM* vm, ValueT* fname)
{
  const static char* METHOD = "open-input-file";
  AssertVT(vm, isstr(fname), fname, "%s: not a string", METHOD);
  StrPtr fn = strref(fname);
  const char* filename = Ssstr(fn);
  FILE* fhandle = fopen(filename, "r");
  if (fhandle == NULL)
  {
    Print("%s: error read file %s", METHOD, filename);
    throw "ReadError: failed to read file";
  }
  InputPortObj* iport = NULL;
  ValueT ret;
  setiport(&ret, iport = Sr0(vm, InputPortObj));
  iport->file = fhandle;
  iport->fname = fn;
  return ret;
}

static ValueT scm_stub_open_output_file(VM* vm, ValueT* fname)
{
  const static char* METHOD = "open-output-file";
  AssertVT(vm, isstr(fname), fname, "%s: not a string", METHOD);
  StrPtr fn = strref(fname);
  const char* filename = Ssstr(fn);
  FILE* fhandle = fopen(filename, "w");
  if (fhandle == NULL)
  {
    Print("%s: error create file %s", METHOD, filename);
    throw "Error: failed to create file";
  }
  OutputPortFileObj* oport = NULL;
  ValueT ret;
  setoport(&ret, oport = Sr0(vm, OutputPortFileObj));
  oport->file = fhandle;
  oport->fname = fn;
  return ret;
}

static ValueT scm_stub_close_input_port(VM* vm, ValueT* vt)
{
  const static char* METHOD = "close-input-port";
  AssertVT(vm, isiport(vt), vt, "%s: not a port", METHOD);
  InputPortObj* iport = iportref(vt);
  iport->close();
  return Svoidref;
}

static ValueT scm_stub_close_output_port(VM* vm, ValueT* vt)
{
  const static char* METHOD = "close-output-port";
  AssertVT(vm, isoport(vt), vt, "%s: not a output port", METHOD);
  OutputPortObj* oport = oportref(vt);
  oport->close();
  return Svoidref;
}

static ValueT scm_stub_flush_output_port(VM* vm, ValueT* args)
{
  const static char* METHOD = "flush-output-port";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (oport)
    oport->flush();
  else
    getcuroport(vm)->flush();
  return Svoidref;
}

static ValueT scm_stub_peek_char(VM* vm, ValueT* args)
{
  const static char* METHOD = "peek-char";
  InputPortObj* iport = getiportrest(vm, args, METHOD);
  if (iport == NULL)
    iport = getcuriport(vm);
  int c = iport->peekchar();
  if (c < 0)
    return Seofref;
  ValueT ret;
  setchar(&ret, c);
  return ret;
}

static ValueT scm_stub_read_char(VM* vm, ValueT* args)
{
  const static char* METHOD = "read-char";
  InputPortObj* iport = getiportrest(vm, args, METHOD);
  if (iport == NULL)
    iport = getcuriport(vm);
  int c = iport->readchar();
  if (c < 0)
    return Seofref;
  ValueT ret;
  setchar(&ret, c);
  return ret;
}

#if defined(SCM_PLATFORM_WINDOWS)
static bool scm_input_ready(FILE* file)
{
  int fd = _fileno(file);
  if (fd < 0)
    return false;
  if (_isatty(fd))
    return _kbhit() != 0;   // console input: key already buffered?
  return true;             // regular file / pipe: data or EOF is available
}
#elif defined(SCM_PLATFORM_APPLE) || defined(SCM_PLATFORM_LINUX)
static bool scm_input_ready(FILE* file)
{
  int fd = fileno(file);
  if (fd < 0)
    return false;
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  if (poll(&pfd, 1, 0) <= 0)
    return false;
  return (pfd.revents & POLLIN) != 0;
}
#endif

static ValueT scm_stub_char_readyp(VM* vm, ValueT* args)
{
  const static char* METHOD = "char-ready?";
  InputPortObj* iport = getiportrest(vm, args, METHOD);
  if (iport == NULL)
    iport = getcuriport(vm);
  if (iport->n < iport->size)
    return Strueref;
  if (iport->eof)
    return Strueref;
#if defined(SCM_PLATFORM_WINDOWS) || defined(SCM_PLATFORM_APPLE) || defined(SCM_PLATFORM_LINUX)
  if (iport->file != NULL && scm_input_ready(iport->file))
    return Strueref;
#endif
  return Sfalseref;
}

static ValueT scm_stub_write_char(VM* vm, ValueT* cvt, ValueT* args)
{
  const static char* METHOD = "write-char";
  AssertVT(vm, ischar(cvt), cvt, "%s: not a char", METHOD);
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  char c = vtchar(cvt);
  if (oport)
    oport->writechar(c);
  else
    G(vm)->oport->writechar(c);
  return Svoidref;
}

static ValueT scm_stub_eof_objp(VM* vm, ValueT* vt)
{
  return frombool(iseof(vt));
}

static ValueT scm_stub_read(VM* vm, ValueT* args)
{
  const static char* METHOD = "read";
  InputPortObj* iport = getiportrest(vm, args, METHOD);
  if (iport == NULL)
    iport = getcuriport(vm);
  Sgcvar1(vm, ret);
  iport->read(vm, ret);
  return ret;
}

void SCMPort::init(VM* vm)
{
  require_get_path(vm);  /* read SCHEME_PATH once at startup */

  const RegCProc port[] = {
    RegCProc("load", scm_stub_load),
    RegCProc("require", scm_stub_require),
    RegCProc("require-clear!", scm_stub_require_clear),
    RegCProc("require-path", scm_stub_require_path),
    RegCProc("require-path!", scm_stub_require_path_set),
    RegCProc("display", scm_stub_display, true),
    RegCProc("write", scm_stub_write, true),
    RegCProc("newline", scm_stub_newline, true),
    RegCProc("port?", scm_stub_portp),
    RegCProc("input-port?", scm_stub_iportp),
    RegCProc("output-port?", scm_stub_oportp),
    RegCProc("current-input-port", scm_stub_current_iport),
    RegCProc("current-output-port", scm_stub_current_oport),
    RegCProc("open-input-file", scm_stub_open_input_file),
    RegCProc("open-output-file", scm_stub_open_output_file),
    RegCProc("close-input-port", scm_stub_close_input_port),
    RegCProc("close-output-port", scm_stub_close_output_port),
    RegCProc("peek-char", scm_stub_peek_char, true),
    RegCProc("read-char", scm_stub_read_char, true),
    RegCProc("char-ready?", scm_stub_char_readyp, true),
    RegCProc("write-char", scm_stub_write_char, true),
    RegCProc("eof-object?", scm_stub_eof_objp),
    RegCProc("read", scm_stub_read, true),
    RegCProc("flush-output-port", scm_stub_flush_output_port, true),
    RegCProc("flush-output", scm_stub_flush_output_port, true),
    RegCProc(NULL, -1)
  };
  regcfunc(vm, port);
}

};

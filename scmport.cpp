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
  CallFrame* fm = Stk(vm)->curfrm;
  if (fm->env.curiport) return fm->env.curiport;
  return vm->iport;
}

static OutputPortObj* getcuroport(VM* vm)
{
  CallFrame* fm = Stk(vm)->curfrm;
  if (fm->env.curoport) return fm->env.curoport;
  return vm->oport;
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
    vm->oport->writechar(c);
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
  const RegCProc port[] = {
    RegCProc("load", scm_stub_load),
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

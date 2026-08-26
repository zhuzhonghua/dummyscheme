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

static ValueT scm_stub_write(VM* vm, ValueT* p, ValueT* args)
{
  const static char* METHOD = "write";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (oport)
    oport->write(vm, p);
  else
    vm->printvalue(p);
  return Svoidref;
}

static ValueT scm_stub_newline(VM* vm, ValueT* args)
{
  const static char* METHOD = "newline";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (oport)
    oport->writechar('\n');
  else
    putc('\n', stderr);
  return Svoidref;
}

static ValueT scm_stub_display(VM* vm, ValueT* p, ValueT* args)
{
  const static char* METHOD = "display";
  OutputPortObj* oport = getoportrest(vm, args, METHOD);
  if (isstr(p))
  {
    StrPtr strp = strref(p);
    if (!oport)
      fwrite(Ssstr(strp), 1, Sslen(strp), stderr);
    else
      oport->writestr(Ssstr(strp), Sslen(strp));
  }
  else if (ischar(p))
  {
    char c = vtchar(p);
    if (!oport)
      putc(c, stderr);
    else
      oport->writechar(c);
  }
  else
  {
    if (!oport)
      vm->printvalue(p);
    else
      oport->write(vm, p);
  }
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

static ValueT scm_stub_current_iport(VM* vm)
{
  ValueT vt;
  setiport(&vt, vm->iport);
  return vt;
}

static ValueT scm_stub_current_oport(VM* vm)
{
  ValueT vt;
  setoport(&vt, vm->oport);
  return vt;
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

static ValueT scm_stub_peek_char(VM* vm, ValueT* vt)
{
  const static char* METHOD = "peek-char";
  AssertVT(vm, isiport(vt), vt, "%s: not a port", METHOD);
  InputPortObj* iport = iportref(vt);
  int c = iport->peekchar();
  if (c < 0)
    return Seofref;
  ValueT ret;
  setchar(&ret, c);
  return ret;
}

static ValueT scm_stub_read_char(VM* vm, ValueT* vt)
{
  const static char* METHOD = "read-char";
  AssertVT(vm, isiport(vt), vt, "%s: not a port", METHOD);
  InputPortObj* iport = iportref(vt);
  int c = iport->readchar();
  if (c < 0)
    return Seofref;
  ValueT ret;
  setchar(&ret, c);
  return ret;
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

static ValueT scm_stub_read(VM* vm, ValueT* vt)
{
  const static char* METHOD = "read";
  AssertVT(vm, isiport(vt), vt, "%s: not a port", METHOD);
  InputPortObj* iport = iportref(vt);
  Sgcvar1(vm, ret);
  iport->read(vm, ret);
  return ret;
}

void SCMPort::init(VM* vm)
{
  const RegCProc port[] = {
    {"load", scm_stub_load},
    {"display", scm_stub_display, true},
    {"write", scm_stub_write, true},
    {"newline", scm_stub_newline, true},
    {"port?", scm_stub_portp},
    {"input-port?", scm_stub_iportp},
    {"output-port?", scm_stub_oportp},
    {"current-output-port", scm_stub_current_oport},
    {"current-input-port", scm_stub_current_iport},
    {"open-input-file", scm_stub_open_input_file},
    {"open-output-file", scm_stub_open_output_file},
    {"close-input-port", scm_stub_close_input_port},
    {"close-output-port", scm_stub_close_output_port},
    {"peek-char", scm_stub_peek_char},
    {"read-char", scm_stub_read_char},
    {"write-char", scm_stub_write_char, true},
    {"eof-object?", scm_stub_eof_objp},
    {"read", scm_stub_read},
    {NULL, -1}
  };
  regcfunc(vm, port);
}

};

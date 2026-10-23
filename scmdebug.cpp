#include "scmdebug.h"

namespace Scheme {

/*
** Fill line information of the closure running in 'frm'.
*/
bool Debug::frameinfo(VM* vm, CallFrame* frm, FrameInfo* info)
{
  (void)vm;
  if (!isclosure(frm->base))
    return false;
  LambdaPtr lambda = closureref(frm->base)->lambda;

  info->source = lambda->source != NULL ? lambda->source->str : "<unknown source>";
  info->currentline = lambda->getline(frm->getpc());
  return true;
}

void Debug::printframe(VM* vm)
{
  fprintf(stderr, "stack traceback:\n");
  for (CallFrame* frm = vm->curfrm; frm != NULL; frm = frm->prev)
    printframe0(vm, frm);
}

void Debug::printframe0(VM* vm, CallFrame* frm)
{
  FrameInfo info;
  if (!frameinfo(vm, frm, &info))
  {
    fprintf(stderr, "\t<native>\n");
    return;
  }

  if (info.currentline > 0)
    fprintf(stderr, "\t%s:%d\n", info.source, info.currentline);
  else
    fprintf(stderr, "\t%s\n", info.source);

  if (frm->tailcall)
    fprintf(stderr, "\t(...tail calls...)\n");
}

}

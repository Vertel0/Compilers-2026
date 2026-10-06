#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

static void readfn(Fn *fn) {
  fillrpo(fn);
  fillpreds(fn);
  filluse(fn);
  ssa(fn);
#ifdef DEBUG
  printfn(fn, stdout);
#endif

  // TODO: implement this assignment's analysis or transformation.

  fillpreds(fn);
  fillrpo(fn);
  printfn(fn, stdout);
}

static void readdat(Dat *dat) { (void)dat; }

int main() {
  parse(stdin, const_cast<char *>("<stdin>"), readdat, readfn);
  freeall();
  return 0;
}

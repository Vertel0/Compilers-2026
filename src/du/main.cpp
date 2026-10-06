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
  // TODO: implement this assignment's analysis or transformation.
}

static void readdat(Dat *dat) { (void)dat; }

int main() {
  parse(stdin, const_cast<char *>("<stdin>"), readdat, readfn);
  freeall();
  return 0;
}

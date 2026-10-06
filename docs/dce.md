# Удаление бесполезного кода

Реализуйте алгоритм удаления бесполезного кода для функций на языке промежуточного представления QBE.

Критическими считайте инструкции записи в память, инструкции возврата из функции и любые вызовы функций.

Решение должно считывать со стандартного потока ввода текстовый файл, содержащий ровно одну функцию на языке промежуточного представления QBE, а на стандартный поток вывода выдавать её же промежуточное представление после преобразования к SSA форме (с помощью libqbe, см. заготовку решения) и удаления бесполезного кода.

Бесполезные инструкции следует заменять инструкциями `nop` без операндов.

## Заготовка решения
```c++
#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

#include <stdio.h>

static int useful(Ins* i) {
    ...
}

static void readfn (Fn *fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    ...
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (Ins *i = blk->ins; i < &blk->ins[b->nins]; ++i) {
            if (!useful(i)) {
                i->op = Onop;
                i->to = R;
                i->arg[0] = R;
                i->arg[1] = R;
            }
        }
        ...
    }

    fillpreds(fn); // Either call this, or keep track of preds manually when rewriting branches.
    fillrpo(fn); // As a side effect, fillrpo removes any unreachable blocks.
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
```

## Пример входной программы и результат работы
### Input-1
```llvm
export function w $bar() {
@start
      %x =w copy 1
      jnz %x, @d1, @d2
@d1
      %y =w copy 1
      %x =w mul %x, %y
      jmp @end
@d2
      %z = w copy 1
      jmp @end
@end
      ret %x
}
```

### Output-1
```llvm
export function $bar() {
@start
        %x.1 =w copy 1
        jnz %x.1, @d1, @d2
@d1
        %y =w copy 1
        %x.3 =w mul %x.1, %y
        jmp @end
@d2
        nop
@end
        %x.2 =w phi @d2 %x.1, @d1 %x.3
        retw %x.2
}
```

### Input-2
```llvm
export function w $foo() {
@start
        %x =w copy 1
        jnz %x, @d1, @d2
@d1
        %y =w copy 1
        jmp @end
@d2
        %z = w copy 1
        jmp @end
@end
        ret %x
}
```

### Output-2
```llvm
export function $foo() {
@start
        %x.1 =w copy 1
@end
        retw %x.1
}
```

### Input-3
```llvm
export function w $baz() {
@start
      %x =w copy 1
      %w =w mul %x, %x
      jnz %x, @d1, @d2
@d1
      %y =w copy 1
      %x =w mul %x, %y
      jmp @end
@d2
      %z = w copy 1
      jmp @end
@end
      ret %w
}
```
### Output-3
```llvm
export function $baz() {
@start
        %x.2 =w copy 1
        %w.3 =w mul %x.2, %x.2
@end
        retw %w.3
}
```

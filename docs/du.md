# Множества Def и Use
Реализуйте алгоритм вычисления множеств `Def` и `Use` для базовых блоков функций на языке промежуточного представления QBE.

Решение должно считывать со стандартного потока ввода текстовый файл, содержащий ровно одну функцию на языке промежуточного представления QBE, а на стандартный поток вывода выдавать для каждого базового блока списки переменных, входящих в соответствующие множества `Def` и `Use`:

```
Output        ::= BlockEntry*
BlockEntry    ::= BlockName Def Use
Def           ::= 'def '=' VariableName*
Use           ::= 'use' '=' VariableName*
```

Имя блока (`BlockName`) определяется как символ '`@`', за которым следует идентификатор блока. Имя переменной (`VariableName`) определяется как символ '`%`', за которым следует идентификатор переменной.

Определение множества `def` следует трактовать так: "`def_B` — множество переменных, определяемых в блоке `B`."

Для работы с промежуточным представлением используйте структуры данных, функции и макросы QBE, объявления и определения которых доступны в заголовочном файле `qbe/all.h`.

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


static void readfn (Fn *fn) {
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tdef = ");

    if (blk->nins && Tmp0 <= blk->ins->to.val) {
      printf("%%%s", fn->tmp[blk->ins->to.val].name);
    }

    printf("\n\tuse =\n\n");
  }
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
```
## Пример работы анализа

### Input-1
```llvm
function w $main() {
@start
    %x =w copy 5
@loop
    %x =w mul %x, %x
    %cond =w csgew %x, 200
    jnz %cond, @end, @loop
@end
    %t =w call $printf(l $fmt, w %x)
    ret 0
}
```

### Output-1
```llvm
@start
	def = %x
	use =

@loop
	def = %x %cond
	use = %x

@end
	def = %t
	use = %x
```
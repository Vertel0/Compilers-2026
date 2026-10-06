# Множества Gen и Kill
Реализуйте алгоритм вычисления множеств `Gen` и `Kill` для базовых блоков функций на языке промежуточного представления QBE.

Решение должно считывать со стандартного потока ввода текстовый файл, содержащий ровно одну функцию на языке промежуточного представления QBE, а на стандартный поток вывода выдавать для каждого базового блока списки определений, входящих в соответствующие множества `Gen` и `Kill`:

```
Output 				::= BlockEntry*
BlockEntry 		::= BlockName Gen Kill
Gen 					::= 'gen' '=' DefinitionName*
Kill 					::= 'kill' '=' DefinitionName*
```

Имя блока (`BlockName`) определяется как символ '`@`', за которым следует идентификатор блока. Имя определения (`DefinitionName`) определяется как имя блока и идентификатор определения, разделённые символом '`%`'.

Для работы с промежуточным представлением используйте структуры данных, функции и макросы QBE, объявления и определения которых доступны в заголовочном файле `qbe/all.h`.

## Заготовка решения
```c++
#include <qbe/all.h>

#include <stdio.h>


static void readfn (Fn *fn) {
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tgen = ");

    if (blk->nins && Tmp0 <= blk->ins->to.val) {
      printf("@%s%%%s", blk->name, fn->tmp[blk->ins->to.val].name);
    }

    printf("\n\tkill =\n\n");
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

## Пример выполнения анализа
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
	gen = @start%x
	kill = @loop%x

@loop
	gen = @loop%x @loop%cond
	kill = @start%x

@end
	gen = @end%t
	kill =
```

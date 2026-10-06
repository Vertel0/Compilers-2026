# Распространение копий
Реализуйте алгоритм распространения копий для функций на языке промежуточного представления QBE.

## Input format
Решение считывает со стандартного потока ввода текст, содержащий ровно одну функцию на языке промежуточного представления QBE.

## Output format
На стандартный поток вывода решение выдает промежуточное представление функции после применения распространения копий.

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

static void readfn(Fn* fn) {
  fillrpo(fn);
  PropagateCopies(fn); // Функция производит распространение копий
  printfn(fn, stdout);
}

static void readdat(Dat* dat) {
  (void) dat;
}

int main() {
  parse(stdin, const_cast<char*>("<stdin>"), readdat, readfn);
  freeall();
}
```

## Пример выполнения трансформации
### Input-1
```llvm
export function w $foo() {
@start
	%x =w copy 1
	%y =w copy %x
	ret %y
}
```

### Output-1
```llvm
export function $foo() {
@start
	%x =w copy 1
	%y =w copy %x
	retw %x
}
```

### Input-2
```llvm
export function w $foo() {
@start
	%a =w copy 1
	%b =w copy 1
@b1
	%c =w add %a, %b
	%d =w copy %c
	%e =w mul %d, %d
@b2
	%f =w add %a, %c
	%g =w copy %e
	%a =w add %g, %d
	%condition =w csltw %a, %c
	jnz %condition, @b3, @b4
@b3
	%h =w add %g, 1
	jmp @b5
@b4
	%f =w sub %d, %g
	%condition2 =w csgtw %f, %a
	jnz %condition2, @b5, @b6
@b5
	%b =w mul %g, %a
	%condition3 =w csgtw %f, %h
	jnz %condition3, @b2, @end
@b6
	%c =w copy 2
	jmp @end
@end
	ret %a
}
```

### Output-2
```llvm
export function $foo() {
@start
	%a =w copy 1
	%b =w copy 1
@b1
	%c =w add %a, %b
	%d =w copy %c
	%e =w mul %c, %c
@b2
	%f =w add %a, %c
	%g =w copy %e
	%a =w add %e, %c
	%condition =w csltw %a, %c
	jnz %condition, @b3, @b4
@b3
	%h =w add %e, 1
	jmp @b5
@b4
	%f =w sub %c, %e
	%condition2 =w csgtw %f, %a
	jnz %condition2, @b5, @b6
@b5
	%b =w mul %e, %a
	%condition3 =w csgtw %f, %h
	jnz %condition3, @b2, @end
@b6
	%c =w copy 2
@end
	retw %a
}
```

## Требования и рекомендация к выполнению
1. Граф потока управления не приводить к SSA-форме.
2. Неиспользуемые инструкции копирования оставить в промежуточном представлении.
3. Базовые блоки не содержат φ-инструкций.
4. Запрещено использовать функцию `void copy(Fn *)`.

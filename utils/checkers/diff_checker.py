def find_diff_pos(expected:str, actual:str):
  line = 1
  col = 1
  for e, a in zip(expected, actual):
    if e != a:
      return line, col
    if e == '\n':
      line += 1
      col = 1
    else:
      col += 1
  return line, col

def diff_check(expected:str, actual:str) -> bool:
  if actual == expected:
    return True

  line, col = find_diff_pos(expected=expected, actual=actual)
  print(
    f"❌ Output mismatch at [Line {line}, Col {col}]\n"
    f"Expected: {repr(expected.splitlines()[line-1])}\n"
    f"Got:      {repr(actual.splitlines()[line-1])}"
  )
  return False

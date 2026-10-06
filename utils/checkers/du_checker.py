from lark import Lark, Transformer
from dataclasses import dataclass

grammar = r"""
    output               : block_entry*

    block_entry          : BLOCK_NAME def_set use_set

    def_set              : "def" "=" VARIABLE_NAME*
    use_set              : "use" "=" VARIABLE_NAME*

    IDENTIFIER           : ("_"|LETTER) ("_"|"."|LETTER|DIGIT)*
    BLOCK_NAME           : "@" IDENTIFIER
    VARIABLE_NAME        :  "%" IDENTIFIER

    %import common.WORD
    %import common.LETTER
    %import common.DIGIT
    %import common.WS
    %ignore WS
"""

@dataclass(frozen=True)
class BasicBlock:
   def_set:set[str]
   use_set:set[str]

class DUTransformer(Transformer):
  def BLOCK_NAME(self, token):
    return str(token)

  def VARIABLE_NAME(self, children):
    return str(children)

  def def_set(self, items):
    return set(items)

  def use_set(self, items):
    return set(items)

  def block_entry(self, items):
    return items[0], BasicBlock(items[1], items[2])

  def output(self, entries):
    return dict(entries)

def du_check(expected, actual):
  parser = Lark(grammar, start='output', parser='lalr')
  transformer = DUTransformer()

  def safe_parse(text, label:str):
    try:
      result = transformer.transform(parser.parse(text))
      return result 
    except Exception as e:
      print(f"❌ Failed to parse {label} output: {e}")
      print(f"{label.capitalize()} output was:")
      print(text)
      return None

  actual_data = safe_parse(actual, "actual")
  expected_data = safe_parse(expected, "expected")

  if actual_data is None or expected_data is None:
    return False

  if actual_data == expected_data:
    return True

  print("❌ Что-то не так:")

  actual_keys = set(actual_data.keys())
  expected_keys = set(expected_data.keys())

  if actual_keys != expected_keys:
    print(f"  - Множества базовых блоков отличаются!")
    print(f"    Лишние: {actual_keys - expected_keys}")
    print(f"    Отсутствуют: {expected_keys - actual_keys}")

  for block in actual_keys & expected_keys:
    actual_use = actual_data[block].use_set
    expected_use = expected_data[block].use_set
    if actual_use != expected_use:
      print(f"  - В базовом блоке {block} не совпадают def:")
      print(f"    Ожидалось: {expected_use}")
      print(f"    Получено:  {actual_use}")

  for block in actual_keys & expected_keys:
    actual_def = actual_data[block].def_set
    expected_def = expected_data[block].def_set
    if actual_def != expected_def:
      print(f"  - В базовом блоке {block} не совпадают use:")
      print(f"    Ожидалось: {expected_def}")
      print(f"    Получено:  {actual_def}")
  return False

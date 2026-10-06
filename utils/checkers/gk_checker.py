from lark import Lark, Transformer
from dataclasses import dataclass

grammar = r"""
    output               : block_entry*

    block_entry          : BLOCK_NAME gen_set kill_set

    gen_set              : "gen" "=" DEFINITION_NAME*
    kill_set             : "kill" "=" DEFINITION_NAME*

    IDENTIFIER           : ("_"|LETTER) ("_"|"."|LETTER|DIGIT)*
    BLOCK_NAME           : "@" IDENTIFIER
    DEFINITION_NAME      : BLOCK_NAME "%" IDENTIFIER

    %import common.WORD
    %import common.LETTER
    %import common.DIGIT
    %import common.WS
    %ignore WS
"""

@dataclass(frozen=True)
class BasicBlock:
   gen:set[str]
   kill:set[str]

class GKTransformer(Transformer):
    def BLOCK_NAME(self, token):
        return str(token)

    def DEFINITION_NAME(self, children):
        return str(children)

    def gen_set(self, items):
        return set(items)

    def kill_set(self, items):
        return set(items)

    def block_entry(self, items):
        return items[0], BasicBlock(items[1], items[2])

    def output(self, entries):
        return dict(entries)

def gk_check(expected, actual):
  parser = Lark(grammar, start='output', parser='lalr')
  transformer = GKTransformer()

  def safe_parse(text, label:str):
    try:
      result = transformer.transform(parser.parse(text))
      return result 
    except Exception as e:
      print(f"❌ Failed to parse {label} output: {e}")
      print(f"{label.capitalize()} output was:")
      print(text)
      return None

  expected_data = safe_parse(expected, "expected")
  actual_data = safe_parse(actual, "actual")

  if expected_data is None or actual_data is None:
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
    if actual_data[block].gen != expected_data[block].gen:
      print(f"  - В базовом блоке {block} не совпадают gen:")
      print(f"    Ожидалось: {expected_data[block].gen}")
      print(f"    Получено:  {actual_data[block].gen}")

  for block in actual_keys & expected_keys:
    if actual_data[block].kill != expected_data[block].kill:
      print(f"  - В базовом блоке {block} не совпадают kill:")
      print(f"    Ожидалось: {expected_data[block].kill}")
      print(f"    Получено:  {actual_data[block].kill}")
  return False

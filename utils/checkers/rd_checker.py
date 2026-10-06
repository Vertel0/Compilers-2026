import sys
from lark import Lark
from lark import Transformer
import argparse

grammar = r"""
    output               : block_entry*

    block_entry          : BLOCK_NAME reaching_definitions

    reaching_definitions : "rd_in" "=" DEFINITION_NAME*

    IDENTIFIER           : ("_"|LETTER) ("_"|"."|LETTER|DIGIT)*
    BLOCK_NAME           : "@" IDENTIFIER
    DEFINITION_NAME      : BLOCK_NAME "%" IDENTIFIER

    %import common.WORD
    %import common.LETTER
    %import common.DIGIT
    %import common.WS
    %ignore WS
"""

class RDTransformer(Transformer):
    def BLOCK_NAME(self, token):
        return str(token)

    def DEFINITION_NAME(self, children):
        return str(children)

    def reaching_definitions(self, items):
        return set(items)

    def block_entry(self, items):
        return items[0], items[1]

    def output(self, entries):
        return dict(entries)

def rd_check(expected, actual):
  parser = Lark(grammar, start='output', parser='lalr')
  transformer = RDTransformer()

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
    if actual_data[block] != expected_data[block]:
      print(f"  - В базовом блоке {block} не совпадают rd_in:")
      print(f"    Ожидалось: {expected_data[block]}")
      print(f"    Получено:  {actual_data[block]}")
  return False

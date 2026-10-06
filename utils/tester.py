import os
import subprocess
import argparse
from typing import Union
from dataclasses import dataclass
from typing import Protocol

from checkers.diff_checker import diff_check
from checkers.du_checker import du_check
from checkers.gk_checker import gk_check
from checkers.lv_checker import lv_check
from checkers.rd_checker import rd_check

CHECKERS = {
  "cp": diff_check,
  "dce": diff_check,
  "du": du_check,
  "gk": gk_check,
  "lv": lv_check,
  "rd": rd_check,
}

class Checker(Protocol):
  def __call__(self, expected: str, actual: str) -> bool: ...

@dataclass (frozen=True)
class Success:
  test_code:str
  stdout:str

@dataclass (frozen=True)
class Error:
  stderr:str
  returncode:int
  test_code:str

def run_tool(tool_path, test_path, tool_args) -> Union[Success, Error]:
  with open(test_path, 'r') as f:
    test_code = f.read()

  cmd = [tool_path] + tool_args

  proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
  stdout, stderr = proc.communicate(input=test_code, timeout=5)

  if proc.returncode != 0:
    return Error(stderr=stderr, returncode=proc.returncode, test_code=test_code)

  return Success(test_code=test_code, stdout=stdout)

def run_suite(bin_path:str, test_suite:str, extra_args, checker:Checker) -> bool:
  files = [f for f in os.listdir(test_suite) if f.endswith('.ssa')]

  if len(files) == 0:
    print("❌ Test is empty!")
    return False

  for fname in sorted(files):
    in_path = os.path.join(test_suite, fname)
    status = run_tool(bin_path, in_path, extra_args)

    match status:
      case Error(stderr, return_code, code):
        message = f"Crash (Exit code {return_code})\n{stderr}"
        print(f"\n❌ {fname}: FAILED")
        print("-" * 40)
        print(f"REASON: {message}")
        print("-" * 40)
        print("FAILED TEST CODE:")
        print(code)
        print("-" * 40 + "\n")
        return False
      case Success(code, stdout):
        actual_output = stdout
        test_code = code

    # in_path -- путь к .ssa файлу
    base_path = os.path.splitext(in_path)[0]
    ans_path = base_path + ".ans"
    if not os.path.exists(ans_path):
      print(f"❌ Missing reference file: {os.path.basename(ans_path)}", test_code)
      return False

    with open(ans_path, 'r') as f:
      expected_output = f.read()
    if not checker(expected=expected_output, actual=actual_output):
      return False

    print(f"✅ {fname}: PASSED")

  return True

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--checker", "-c", 
                       choices=CHECKERS.keys(),
                       help='Checker function to use')
  parser.add_argument("--bin-path", "-b")
  parser.add_argument("--tests-dir", "-t")
  parser.add_argument("extra_args", nargs=argparse.REMAINDER, help="Args for the compiler")
  args = parser.parse_args()

  check_func = CHECKERS[args.checker]
  success = run_suite(args.bin_path, args.tests_dir, args.extra_args, check_func)

  if not success:
    exit(1)

if __name__ == "__main__":
  main()
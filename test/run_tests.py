#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
import difflib
import argparse
import re

# Colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
RESET = "\033[0m"

def print_color(text, color):
    print(f"{color}{text}{RESET}")

def run_command(cmd, capture_output=True):
    try:
        res = subprocess.run(cmd, shell=True, capture_output=capture_output, text=True)
        return res.returncode, res.stdout, res.stderr
    except Exception as e:
        return -1, "", str(e)

def get_file_content(path):
    if not os.path.exists(path):
        return None
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        return f.read()

def sanitize_stderr(text):
    # Strip ANSI escape sequences first so we work with plain text
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    text = ansi_escape.sub('', text)
    
    lines = text.splitlines()
    sanitized = []
    for line in lines:
        # Strip TTY/ioctl warnings
        if "ioctl() failed" in line:
            continue
        # Strip sassy error box elements (borders, box characters, emoji)
        if any(c in line for c in ["😡", "╭", "╰", "─", "⎨", "╯"]):
            continue
        # Strip lines showing the sassy message inside vertical borders
        if "│" in line and any(msg in line for msg in [
            "Why is this line so long", "This line is longer", "novel in one line"
        ]):
            continue
            
        # Clean up funny message underlines (remove randomized trailing caret)
        # e.g. "     │              ^^                                 ^"
        # We look for a block of carets followed by spaces and a trailing caret at the end
        line = re.sub(r'(\^+)\s+\^$', r'\1', line)
        
        sanitized.append(line)
    return "\n".join(sanitized)

def diff_files(expected_path, actual_path):
    expected = get_file_content(expected_path)
    actual = get_file_content(actual_path)
    
    if expected is None:
        return f"Expected file {expected_path} does not exist."
    if actual is None:
        return f"Actual file {actual_path} does not exist."
        
    if expected == actual:
        return None
        
    diff = list(difflib.unified_diff(
        expected.splitlines(keepends=True),
        actual.splitlines(keepends=True),
        fromfile=expected_path,
        tofile=actual_path
    ))
    return "".join(diff)

def main():
    parser = argparse.ArgumentParser(description="GigglyCode Test Runner")
    parser.add_argument("-g", "--generate", action="store_true", help="Generate/Update ground truth (expected) files")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    compiler_bin = os.path.join(project_root, "build", "gigly")

    if not os.path.exists(compiler_bin):
        print_color(f"Compiler binary not found at {compiler_bin}. Please run 'cmake --build build' first.", RED)
        sys.exit(1)

    test_dir = os.path.join(project_root, "test")
    test_folders = sorted([
        f for f in os.listdir(test_dir)
        if os.path.isdir(os.path.join(test_dir, f)) and f.startswith("test")
    ])

    failed_tests = []
    passed_tests = []

    for folder in test_folders:
        folder_path = os.path.join(test_dir, folder)
        is_fail_test = folder.startswith("testf")
        
        print_color(f"\n--- Running test: {folder} ({'Expected to Fail' if is_fail_test else 'Expected to Pass'}) ---", BLUE)
        
        # Clean build dir in test case
        build_dir = os.path.join(folder_path, "build")
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
        
        expected_dir = os.path.join(folder_path, "expected")
        if args.generate and not os.path.exists(expected_dir):
            os.makedirs(expected_dir)

        # 1. Compile
        exec_path = os.path.join(build_dir, "exec")
        compile_cmd = f"{compiler_bin} {folder_path} -o {exec_path}"
        if args.verbose:
            print(f"Running command: {compile_cmd}")
            
        ret_code, stdout, stderr = run_command(compile_cmd)
        
        if is_fail_test:
            # Compiler should fail
            if ret_code == 0:
                print_color(f"[{folder}] FAILED: Expected compilation to fail, but it succeeded.", RED)
                failed_tests.append(folder)
                continue
            
            sanitized_err = sanitize_stderr(stderr)
            
            if args.generate:
                with open(os.path.join(expected_dir, "stderr.txt"), "w") as f:
                    f.write(sanitized_err)
                print_color(f"[{folder}] Stderr generated.", GREEN)
                passed_tests.append(folder)
            else:
                expected_stderr_path = os.path.join(expected_dir, "stderr.txt")
                if not os.path.exists(expected_stderr_path):
                    print_color(f"[{folder}] FAILED: No expected/stderr.txt found.", RED)
                    failed_tests.append(folder)
                else:
                    # Compare sanitized stderr
                    expected_err = get_file_content(expected_stderr_path)
                    if expected_err.strip() == sanitized_err.strip():
                        print_color(f"[{folder}] PASSED: Compilation failed as expected with matching stderr.", GREEN)
                        passed_tests.append(folder)
                    else:
                        print_color(f"[{folder}] FAILED: Compilation failed, but stderr did not match.", RED)
                        diff = "".join(difflib.unified_diff(
                            expected_err.splitlines(keepends=True),
                            sanitized_err.splitlines(keepends=True),
                            fromfile="expected_stderr",
                            tofile="actual_stderr"
                        ))
                        print(diff)
                        failed_tests.append(folder)
            continue
            
        # For compile-pass tests:
        if ret_code != 0:
            print_color(f"[{folder}] FAILED: Compilation failed with exit code {ret_code}.\nStderr: {stderr}", RED)
            failed_tests.append(folder)
            continue

        # Paths of outputs
        lexer_log = os.path.join(build_dir, "lexer_output.log")
        parser_yaml = os.path.join(build_dir, "parser_output.yaml")
        ll_file = os.path.join(build_dir, "ir", "main.gc.ll")
        
        # Check that compiler outputs exist
        if not os.path.exists(lexer_log) or not os.path.exists(parser_yaml) or not os.path.exists(ll_file):
            print_color(f"[{folder}] FAILED: Missing output files in build directory.", RED)
            failed_tests.append(folder)
            continue

        # 2. Run Executable
        run_cmd_str = exec_path
        if args.verbose:
            print(f"Running executable: {run_cmd_str}")
        exec_ret, exec_stdout, exec_stderr = run_command(run_cmd_str)
        
        # 3. Generate or Verify
        if args.generate:
            shutil.copy(lexer_log, os.path.join(expected_dir, "lexer_output.log"))
            shutil.copy(parser_yaml, os.path.join(expected_dir, "parser_output.yaml"))
            shutil.copy(ll_file, os.path.join(expected_dir, "main.gc.ll"))
            with open(os.path.join(expected_dir, "stdout.txt"), "w") as f:
                f.write(exec_stdout)
            print_color(f"[{folder}] Ground truth files generated.", GREEN)
            passed_tests.append(folder)
        else:
            # Compare files
            lexer_diff = diff_files(os.path.join(expected_dir, "lexer_output.log"), lexer_log)
            parser_diff = diff_files(os.path.join(expected_dir, "parser_output.yaml"), parser_yaml)
            ll_diff = diff_files(os.path.join(expected_dir, "main.gc.ll"), ll_file)
            
            # Compare stdout
            expected_stdout_path = os.path.join(expected_dir, "stdout.txt")
            expected_stdout = get_file_content(expected_stdout_path)
            stdout_diff = None
            if expected_stdout is not None:
                if expected_stdout != exec_stdout:
                    stdout_diff = "".join(difflib.unified_diff(
                        expected_stdout.splitlines(keepends=True),
                        exec_stdout.splitlines(keepends=True),
                        fromfile="expected_stdout",
                        tofile="actual_stdout"
                    ))
            
            failed_reasons = []
            if lexer_diff:
                failed_reasons.append("lexer_output.log mismatch")
                if args.verbose:
                    print_color("\n--- Lexer Log Diff ---", YELLOW)
                    print(lexer_diff)
            if parser_diff:
                failed_reasons.append("parser_output.yaml mismatch")
                if args.verbose:
                    print_color("\n--- Parser YAML Diff ---", YELLOW)
                    print(parser_diff)
            if ll_diff:
                failed_reasons.append("LLVM IR main.gc.ll mismatch")
                if args.verbose:
                    print_color("\n--- LLVM IR Diff ---", YELLOW)
                    print(ll_diff)
            if stdout_diff:
                failed_reasons.append("execution stdout mismatch")
                if args.verbose:
                    print_color("\n--- Stdout Diff ---", YELLOW)
                    print(stdout_diff)
            if exec_ret != 0:
                failed_reasons.append(f"execution failed with exit code {exec_ret}")
                
            if failed_reasons:
                print_color(f"[{folder}] FAILED: {', '.join(failed_reasons)}", RED)
                # If not verbose, print diffs anyway
                if not args.verbose:
                    if lexer_diff:
                        print_color("\n--- Lexer Log Diff ---", YELLOW)
                        print(lexer_diff)
                    if parser_diff:
                        print_color("\n--- Parser YAML Diff ---", YELLOW)
                        print(parser_diff)
                    if ll_diff:
                        print_color("\n--- LLVM IR Diff ---", YELLOW)
                        print(ll_diff)
                    if stdout_diff:
                        print_color("\n--- Stdout Diff ---", YELLOW)
                        print(stdout_diff)
                failed_tests.append(folder)
            else:
                print_color(f"[{folder}] PASSED: All outputs match.", GREEN)
                passed_tests.append(folder)

    print("\n================ TEST SUMMARY ================")
    print_color(f"Passed: {len(passed_tests)}/{len(test_folders)}", GREEN if len(passed_tests) == len(test_folders) else YELLOW)
    if failed_tests:
        print_color(f"Failed: {len(failed_tests)}/{len(test_folders)} ({', '.join(failed_tests)})", RED)
        sys.exit(1)
    else:
        print_color("All tests passed successfully!", GREEN)
        sys.exit(0)

if __name__ == "__main__":
    main()

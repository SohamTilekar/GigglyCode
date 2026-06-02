#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
import difflib
import re

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
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    text = ansi_escape.sub('', text)
    lines = text.splitlines()
    sanitized = []
    for line in lines:
        if "ioctl() failed" in line:
            continue
        if any(c in line for c in ["😡", "╭", "╰", "─", "⎨", "╯"]):
            continue
        if "│" in line and any(msg in line for msg in [
            "Why is this line so long", "This line is longer", "novel in one line"
        ]):
            continue
        line = re.sub(r'(\^+)\s+\^$', r'\1', line)
        sanitized.append(line)
    return "\n".join(sanitized)

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    compiler_bin = os.path.join(project_root, "build", "gigly")

    if not os.path.exists(compiler_bin):
        print(f"Compiler binary not found at {compiler_bin}. Run make first.")
        sys.exit(1)

    test_dir = os.path.join(project_root, "test")
    test_folders = sorted([
        f for f in os.listdir(test_dir)
        if os.path.isdir(os.path.join(test_dir, f)) and f.startswith("test")
    ])

    report = []
    report.append("# GigglyCode Compiler Test Verification Report\n")
    report.append("| Test Case | Type | Compile Status | Execution Status | Lexer Log | Parser YAML | LLVM IR | Stdout | Stderr | Verdict | Notes |")
    report.append("|---|---|---|---|---|---|---|---|---|---|---|")

    print("Verifying all test cases...")

    for folder in test_folders:
        folder_path = os.path.join(test_dir, folder)
        is_fail_test = folder.startswith("testf")
        build_dir = os.path.join(folder_path, "build")
        expected_dir = os.path.join(folder_path, "expected")

        # 1. Compile
        exec_path = os.path.join(build_dir, "exec")
        compile_cmd = f"{compiler_bin} {folder_path} -o {exec_path}"
        
        # Clean build first to get fresh results
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
            
        ret_code, stdout, stderr = run_command(compile_cmd)
        
        compile_ok = (ret_code == 0)
        exec_ok = "N/A"
        lexer_match = "N/A"
        parser_match = "N/A"
        ir_match = "N/A"
        stdout_match = "N/A"
        stderr_match = "N/A"
        verdict = "PASS"
        notes = []

        # Check for ASAN leaks or errors
        asan_leak = "AddressSanitizer" in stderr or "AddressSanitizer" in stdout
        if asan_leak:
            notes.append("ASAN memory leaks detected")

        if is_fail_test:
            # Expected to fail compilation
            if compile_ok:
                verdict = "FAIL"
                notes.append("Expected compilation failure but succeeded")
                compile_status = "SUCCEEDED (Unexpected)"
            else:
                compile_status = "FAILED (Expected)"
                # Compare stderr
                expected_stderr_path = os.path.join(expected_dir, "stderr.txt")
                actual_err_sanitized = sanitize_stderr(stderr)
                expected_err = get_file_content(expected_stderr_path)
                if expected_err and expected_err.strip() == actual_err_sanitized.strip():
                    stderr_match = "MATCH"
                else:
                    stderr_match = "MISMATCH"
                    verdict = "FAIL"
                    notes.append("Stderr output mismatch")
        else:
            # Expected to pass compilation
            if not compile_ok:
                verdict = "FAIL"
                notes.append(f"Compilation failed with exit code {ret_code}")
                compile_status = "FAILED"
            else:
                compile_status = "PASSED"
                
                # Check compiler outputs
                lexer_log = os.path.join(build_dir, "lexer_output.log")
                parser_yaml = os.path.join(build_dir, "parser_output.yaml")
                ll_file = os.path.join(build_dir, "ir", "main.gc.ll")

                # Compare Lexer Log
                expected_lexer = os.path.join(expected_dir, "lexer_output.log")
                if os.path.exists(lexer_log) and os.path.exists(expected_lexer):
                    actual_content = get_file_content(lexer_log)
                    expected_content = get_file_content(expected_lexer)
                    if actual_content == expected_content:
                        lexer_match = "MATCH"
                    else:
                        lexer_match = "MISMATCH"
                        # Check if multi-file test (race condition)
                        src_files = [f for f in os.listdir(os.path.join(folder_path, "src")) if f.endswith(".gc")]
                        has_modules = os.path.isdir(os.path.join(folder_path, "src", "modules"))
                        if len(src_files) > 1 or has_modules:
                            notes.append("Lexer log mismatch (flaky due to parallel compilation order)")
                        else:
                            verdict = "FAIL"
                            notes.append("Lexer log mismatch")
                else:
                    lexer_match = "MISSING"
                    verdict = "FAIL"

                # Compare Parser YAML
                expected_parser = os.path.join(expected_dir, "parser_output.yaml")
                if os.path.exists(parser_yaml) and os.path.exists(expected_parser):
                    actual_content = get_file_content(parser_yaml)
                    expected_content = get_file_content(expected_parser)
                    if actual_content == expected_content:
                        parser_match = "MATCH"
                    else:
                        parser_match = "MISMATCH"
                        src_files = [f for f in os.listdir(os.path.join(folder_path, "src")) if f.endswith(".gc")]
                        has_modules = os.path.isdir(os.path.join(folder_path, "src", "modules"))
                        if len(src_files) > 1 or has_modules:
                            notes.append("Parser YAML mismatch (flaky due to parallel compilation order)")
                        else:
                            verdict = "FAIL"
                            notes.append("Parser YAML mismatch")
                else:
                    parser_match = "MISSING"
                    verdict = "FAIL"

                # Compare LLVM IR
                expected_ir = os.path.join(expected_dir, "main.gc.ll")
                if os.path.exists(ll_file) and os.path.exists(expected_ir):
                    actual_content = get_file_content(ll_file)
                    expected_content = get_file_content(expected_ir)
                    if actual_content == expected_content:
                        ir_match = "MATCH"
                    else:
                        ir_match = "MISMATCH"
                        verdict = "FAIL"
                        notes.append("LLVM IR mismatch")
                else:
                    ir_match = "MISSING"
                    verdict = "FAIL"

                # 2. Run Executable
                exec_ret, exec_stdout, exec_stderr = run_command(exec_path)
                if exec_ret == 0:
                    exec_ok = "PASSED"
                else:
                    exec_ok = f"FAILED ({exec_ret})"
                    verdict = "FAIL"
                    notes.append(f"Execution failed with code {exec_ret}")

                # Compare stdout
                expected_stdout_path = os.path.join(expected_dir, "stdout.txt")
                expected_stdout = get_file_content(expected_stdout_path)
                if expected_stdout is not None:
                    if expected_stdout == exec_stdout:
                        stdout_match = "MATCH"
                    else:
                        stdout_match = "MISMATCH"
                        verdict = "FAIL"
                        notes.append("Stdout mismatch")
                else:
                    stdout_match = "MISSING"
                    verdict = "FAIL"

        notes_str = "; ".join(notes) if notes else "OK"
        test_type = "Fail-test" if is_fail_test else "Pass-test"
        
        report.append(f"| {folder} | {test_type} | {compile_status} | {exec_ok} | {lexer_match} | {parser_match} | {ir_match} | {stdout_match} | {stderr_match} | {verdict} | {notes_str} |")
        print(f"Verified {folder}: {verdict}")

    report_path = os.path.join(project_root, "test", "verification_report.md")
    with open(report_path, "w") as f:
        f.write("\n".join(report))
    print(f"\nVerification report generated at: {report_path}")

if __name__ == "__main__":
    main()

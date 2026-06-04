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

def get_normalized_triple(triple):
    if not triple:
        return "unknown"
    parts = triple.split('-')
    if len(parts) >= 3:
        # Ignore vendor part (parts[1])
        return parts[0] + '-' + '-'.join(parts[2:])
    return triple

def get_normalized_file_content(path):
    content = get_file_content(path)
    if content is None:
        return None
    if path.endswith(".ll"):
        lines = []
        for line in content.splitlines():
            # Strip target triple line to ignore vendor differences inside the file
            if line.strip().startswith("target triple ="):
                continue
            lines.append(line)
        return "\n".join(lines)
    return content

def diff_files(expected_path, actual_path):
    expected = get_normalized_file_content(expected_path)
    actual = get_normalized_file_content(actual_path)
    
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
    parser.add_argument(
        "--cross-targets",
        nargs="+",
        default=["aarch64-unknown-linux-gnu", "wasm32-unknown-wasi", "x86_64-unknown-linux-gnu", "s390x-unknown-linux-gnu"],
        metavar="TRIPLE",
        help=(
            "Additional target triples to generate expected IR for during --generate or verify during testing. "
            "Example: --cross-targets aarch64-unknown-linux-gnu wasm32-unknown-wasi x86_64-unknown-linux-gnu s390x-unknown-linux-gnu"
        ),
    )
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
        lexer_log   = os.path.join(build_dir, "lexer_output.log")
        parser_yaml = os.path.join(build_dir, "parser_output.yaml")
        ir_dir      = os.path.join(build_dir, "ir")         # whole IR tree
        main_ll     = os.path.join(ir_dir, "main.gc.ll")    # used only for triple extraction

        # Check that compiler outputs exist
        if not os.path.exists(lexer_log) or not os.path.exists(parser_yaml) or not os.path.exists(main_ll):
            print_color(f"[{folder}] FAILED: Missing output files in build directory.", RED)
            failed_tests.append(folder)
            continue

        # 2. Run Executable
        run_cmd_str = exec_path
        if args.verbose:
            print(f"Running executable: {run_cmd_str}")
        exec_ret, exec_stdout, exec_stderr = run_command(run_cmd_str)

        # 3. Extract target triple from main.gc.ll
        target_triple = None
        main_ll_content = get_file_content(main_ll)
        if main_ll_content:
            for line in main_ll_content.splitlines():
                if line.startswith('target triple ='):
                    parts = line.split('"')
                    if len(parts) >= 2:
                        target_triple = parts[1]
                    break

        if not target_triple:
            print_color(
                f"[{folder}] FAILED: Could not extract target triple from generated IR.\n"
                f"  The compiler may not be setting the target triple correctly.",
                RED,
            )
            failed_tests.append(folder)
            continue

        # Layout:
        #   expected/                        ← target-independent
        #     lexer_output.log
        #     parser_output.yaml
        #     stdout.txt
        #     ir/<normalized_triple>/        ← target-specific, mirrors build/ir/
        #       main.gc.ll
        #       modules/math_utils.gc.ll
        #       ...
        target_norm          = get_normalized_triple(target_triple)
        target_ir_dir        = os.path.join(expected_dir, "ir", target_norm)
        stdout_expected      = os.path.join(expected_dir, "stdout.txt")

        # ── Collect all actual .ll files (relative to ir_dir) ─────────────────
        actual_ll_files = []
        for root, _, files in os.walk(ir_dir):
            for fname in files:
                if fname.endswith(".ll"):
                    full = os.path.join(root, fname)
                    rel  = os.path.relpath(full, ir_dir)
                    actual_ll_files.append(rel)
        actual_ll_files.sort()

        if args.generate:
            # ── 1. Native target: capture IR + stdout + shared files ──────────
            # Wipe and rebuild the target IR tree so stale files are removed
            if os.path.exists(target_ir_dir):
                shutil.rmtree(target_ir_dir)
            shutil.copytree(ir_dir, target_ir_dir)

            # target-independent files at root expected/
            shutil.copy(lexer_log,   os.path.join(expected_dir, "lexer_output.log"))
            shutil.copy(parser_yaml, os.path.join(expected_dir, "parser_output.yaml"))
            with open(stdout_expected, "w") as f:
                f.write(exec_stdout)

            ll_list = "\n    ".join(actual_ll_files)
            print_color(
                f"[{folder}] Ground truth generated for target '{target_triple}' (normalized: '{target_norm}')."
                f"\n  IR files -> expected/ir/{target_norm}/"
                f"\n    {ll_list}"
                f"\n  stdout   -> expected/stdout.txt (target-independent)",
                GREEN,
            )

            # ── 2. Cross-targets: only IR (no execution possible) ─────────────
            for cross_triple in args.cross_targets:
                cross_build_ir = os.path.join(build_dir, f"ir_cross_{cross_triple.replace('-', '_')}")
                cross_compile_cmd = (
                    f"{compiler_bin} {folder_path} -o {exec_path}"
                    f" --target {cross_triple}"
                )
                if args.verbose:
                    print(f"  Cross-compiling for {cross_triple}: {cross_compile_cmd}")

                # Recompile with overridden target (IR-only; we discard the linked exec)
                c_ret, _, c_err = run_command(cross_compile_cmd)
                if c_ret != 0:
                    print_color(
                        f"  [cross:{cross_triple}] FAILED to compile: {c_err.strip()}",
                        RED,
                    )
                    continue

                cross_ir_src = os.path.join(build_dir, "ir")
                cross_norm = get_normalized_triple(cross_triple)
                cross_expected_ir = os.path.join(expected_dir, "ir", cross_norm)
                if os.path.exists(cross_expected_ir):
                    shutil.rmtree(cross_expected_ir)
                shutil.copytree(cross_ir_src, cross_expected_ir)

                cross_ll_files = sorted(
                    os.path.relpath(os.path.join(r, f), cross_ir_src)
                    for r, _, fs in os.walk(cross_ir_src) for f in fs if f.endswith(".ll")
                )
                ll_list_cross = "\n    ".join(cross_ll_files)
                print_color(
                    f"  [cross:{cross_triple}] IR generated -> expected/ir/{cross_norm}/"
                    f"\n    {ll_list_cross}"
                    f"\n  IMPORTANT: Manually verify before committing.",
                    YELLOW,
                )

            print_color(
                "  IMPORTANT: Please manually verify the generated outputs before committing.",
                GREEN,
            )
            passed_tests.append(folder)
        else:
            # ── Hard-fail when no IR tree exists for this target ──────────────
            if not os.path.exists(target_ir_dir):
                print_color(
                    f"[{folder}] SKIPPED / NO EXPECTED IR for target '{target_triple}' (normalized: '{target_norm}').\n"
                    f"  Expected IR directory not found: expected/ir/{target_norm}/\n"
                    f"  This target has not been verified yet on this machine.\n"
                    f"  Run:  make test-gen\n"
                    f"  Then manually verify 'test/{folder}/expected/ir/{target_norm}/'\n"
                    f"  and commit it if the output looks correct.",
                    YELLOW,
                )
                failed_tests.append(folder)
                continue

            # ── Compare all .ll files ─────────────────────────────────────────
            lexer_diff  = diff_files(os.path.join(expected_dir, "lexer_output.log"), lexer_log)
            parser_diff = diff_files(os.path.join(expected_dir, "parser_output.yaml"), parser_yaml)

            ll_diffs = {}   # rel_path -> diff string
            # Check every expected IR file still matches
            for rel in sorted(os.listdir(target_ir_dir) and
                              [os.path.relpath(os.path.join(r, f), target_ir_dir)
                               for r, _, fs in os.walk(target_ir_dir) for f in fs
                               if f.endswith(".ll")]):
                expected_ll = os.path.join(target_ir_dir, rel)
                actual_ll   = os.path.join(ir_dir, rel)
                d = diff_files(expected_ll, actual_ll)
                if d:
                    ll_diffs[rel] = d
            # Also flag any new .ll files not in expected
            expected_rels = set(
                os.path.relpath(os.path.join(r, f), target_ir_dir)
                for r, _, fs in os.walk(target_ir_dir) for f in fs if f.endswith(".ll")
            )
            for rel in actual_ll_files:
                if rel not in expected_rels and rel not in ll_diffs:
                    ll_diffs[rel] = f"NEW FILE not in expected: {rel}"

            # stdout is target-independent
            expected_stdout_content = get_file_content(stdout_expected)
            stdout_diff = None
            if expected_stdout_content is not None:
                if expected_stdout_content != exec_stdout:
                    stdout_diff = "".join(difflib.unified_diff(
                        expected_stdout_content.splitlines(keepends=True),
                        exec_stdout.splitlines(keepends=True),
                        fromfile="expected_stdout",
                        tofile="actual_stdout",
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
            if ll_diffs:
                failed_reasons.append(
                    f"{len(ll_diffs)} LLVM IR file(s) mismatched for target '{target_triple}': "
                    + ", ".join(ll_diffs.keys())
                )
                if args.verbose:
                    for rel, d in ll_diffs.items():
                        print_color(f"\n--- LLVM IR Diff: {rel} ({target_triple}) ---", YELLOW)
                        print(d)
            if stdout_diff:
                failed_reasons.append("execution stdout mismatch")
                if args.verbose:
                    print_color("\n--- Stdout Diff ---", YELLOW)
                    print(stdout_diff)
            if exec_ret != 0:
                failed_reasons.append(f"execution failed with exit code {exec_ret}")

            # ── Cross-targets validation ──────────────────────────────────────
            cross_ll_diffs_all = {}
            for cross_triple in args.cross_targets:
                cross_norm = get_normalized_triple(cross_triple)
                cross_expected_ir = os.path.join(expected_dir, "ir", cross_norm)
                if not os.path.exists(cross_expected_ir):
                    failed_reasons.append(f"missing expected IR for cross-target '{cross_triple}' (normalized: '{cross_norm}')")
                    continue

                # Run the compiler for the cross-target
                cross_compile_cmd = (
                    f"{compiler_bin} {folder_path} -o {exec_path}"
                    f" --target {cross_triple}"
                )
                if args.verbose:
                    print(f"  Testing cross-target {cross_triple}: {cross_compile_cmd}")

                c_ret, _, c_err = run_command(cross_compile_cmd)
                if c_ret != 0:
                    failed_reasons.append(f"cross-compilation for target '{cross_triple}' failed with: {c_err.strip()}")
                    continue

                # Check generated IR files against expected IR files for this cross-target
                cross_ir_src = os.path.join(build_dir, "ir")

                # Collect all actual cross .ll files
                cross_actual_ll_files = []
                for root, _, files in os.walk(cross_ir_src):
                    for fname in files:
                        if fname.endswith(".ll"):
                            full = os.path.join(root, fname)
                            rel  = os.path.relpath(full, cross_ir_src)
                            cross_actual_ll_files.append(rel)
                cross_actual_ll_files.sort()

                # Diff them
                cross_ll_diffs = {}
                for rel in sorted(os.listdir(cross_expected_ir) and
                                  [os.path.relpath(os.path.join(r, f), cross_expected_ir)
                                   for r, _, fs in os.walk(cross_expected_ir) for f in fs
                                   if f.endswith(".ll")]):
                    expected_ll = os.path.join(cross_expected_ir, rel)
                    actual_ll   = os.path.join(cross_ir_src, rel)
                    d = diff_files(expected_ll, actual_ll)
                    if d:
                        cross_ll_diffs[rel] = d

                expected_rels_cross = set(
                    os.path.relpath(os.path.join(r, f), cross_expected_ir)
                    for r, _, fs in os.walk(cross_expected_ir) for f in fs if f.endswith(".ll")
                )
                for rel in cross_actual_ll_files:
                    if rel not in expected_rels_cross and rel not in cross_ll_diffs:
                        cross_ll_diffs[rel] = f"NEW FILE not in expected: {rel}"

                if cross_ll_diffs:
                    cross_ll_diffs_all[cross_triple] = cross_ll_diffs
                    failed_reasons.append(
                        f"{len(cross_ll_diffs)} LLVM IR file(s) mismatched for cross-target '{cross_triple}': "
                        + ", ".join(cross_ll_diffs.keys())
                    )
                    if args.verbose:
                        for rel, d in cross_ll_diffs.items():
                            print_color(f"\n--- LLVM IR Diff: {rel} ({cross_triple}) ---", YELLOW)
                            print(d)

            if failed_reasons:
                print_color(f"[{folder}] FAILED: {', '.join(failed_reasons)}", RED)
                if not args.verbose:
                    if lexer_diff:
                        print_color("\n--- Lexer Log Diff ---", YELLOW)
                        print(lexer_diff)
                    if parser_diff:
                        print_color("\n--- Parser YAML Diff ---", YELLOW)
                        print(parser_diff)
                    if ll_diffs:
                        for rel, d in ll_diffs.items():
                            print_color(f"\n--- LLVM IR Diff: {rel} ({target_triple}) ---", YELLOW)
                            print(d)
                    if stdout_diff:
                        print_color("\n--- Stdout Diff ---", YELLOW)
                        print(stdout_diff)
                    for cross_triple, diffs in cross_ll_diffs_all.items():
                        for rel, d in diffs.items():
                            print_color(f"\n--- LLVM IR Diff: {rel} ({cross_triple}) ---", YELLOW)
                            print(d)
                failed_tests.append(folder)
            else:
                ir_count = len(actual_ll_files)
                cross_msg = f", {len(args.cross_targets)} cross-target(s)" if args.cross_targets else ""
                print_color(
                    f"[{folder}] PASSED: All outputs match "
                    f"(target: {target_triple}{cross_msg}, {ir_count} IR file(s)).",
                    GREEN,
                )
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

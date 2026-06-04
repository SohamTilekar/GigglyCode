.PHONY: run runr orun lldb gdb cppcheck format test test-gen test-gen-cross configure-debug configure-release build-debug build-release

configure-debug:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug

configure-release:
	cmake -B build -DCMAKE_BUILD_TYPE=Release

build-debug:
	cmake --build ./build --config Debug --target all -j 4

build-release:
	cmake --build ./build --config Release --target all -j 4

run: build-debug
	@echo ---------------------------------------------------------
	./build/gigly ./dump/ -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec $(ARGS)

runr: build-release
	@echo ---------------------------------------------------------
	./build/gigly ./dump/ -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec $(ARGS)

orun: build-debug
	@echo ---------------------------------------------------------
	./build/gigly ./dump/ -O3 -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

lldb: build-debug
	lldb ./build/gigly

gdb: build-debug
	gdb ./build/gigly

cppcheck:
	cppcheck --enable=all --check-level=exhaustive --error-exitcode=1 src/ -i src/include/ -I src/ --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=noExplicitConstructor --force

format:
	clang-format -i src/compiler/compiler.cpp src/compiler/compiler.hpp src/compiler/enviornment/enviornment.cpp src/compiler/enviornment/enviornment.hpp src/errors/errors.cpp src/errors/errors.hpp src/lexer/lexer.cpp src/lexer/lexer.hpp src/lexer/token.cpp src/lexer/token.hpp src/parser/AST/ast.cpp src/parser/AST/ast.hpp src/parser/parser.cpp src/parser/parser.hpp src/gigly.cpp src/gigc.cpp

test: build-debug
	python3 test/run_tests.py

test-gen: build-debug
	python3 test/run_tests.py -g

# Generate expected IR for common cross-compilation targets.
# Uses LLVM's cross-target support — no external toolchain needed.
# Add/remove triples as required.
CROSS_TARGETS ?= aarch64-unknown-linux-gnu wasm32-unknown-wasi x86_64-unknown-linux-gnu s390x-unknown-linux-gnu
test-gen-cross: build-debug
	python3 test/run_tests.py -g --cross-targets $(CROSS_TARGETS)

setenv:
	export GC_STD_OBJ="/mnt/soham/soham_code/GigglyCode/dump/std.o"
	export GC_STD_DIR="/mnt/soham/soham_code/GigglyCode/std/src/"
	export GC_STD_IRGCMAP="/mnt/soham/soham_code/GigglyCode/std/build/ir_gc_map/"
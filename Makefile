run:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./test/
	@echo ---------------------------------------------------------
	clang ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

orun:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./test/
	@echo ---------------------------------------------------------
	clang -O3 ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

ir:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	./build/gigly ./test/

oir:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	./build/gigly ./test/
	@echo ---------------------------------------------------------
	clang -emit-llvm -S -O3 -v ./dump/testscipt.ll -o ./dump/exec.ir

rnir:
	@echo ---------------------------------------------------------
	clang ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

lldb:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	lldb ./build/gigly

gdb:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	gdb ./build/gigly
cppcheck:
	cppcheck --enable=all --check-level=exhaustive --error-exitcode=1 src/ -i src/include/json.hpp -I src/ --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --force

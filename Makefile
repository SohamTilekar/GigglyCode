run:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all --
	@echo ---------------------------------------------------------
	./build/gigly ./test/testscipt.gc -o ./dump/testscipt.ll
	@echo ---------------------------------------------------------
	clang ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

orun:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all --
	@echo ---------------------------------------------------------
	./build/gigly ./test/testscipt.gc -o ./dump/testscipt.ll
	@echo ---------------------------------------------------------
	clang -O3 ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

ir:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all --
	./build/gigly ./test/testscipt.gc -o ./dump/testscipt.ll

oir:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all --
	./build/gigly ./test/testscipt.gc -o ./dump/testscipt.ll
	@echo ---------------------------------------------------------
	clang -emit-llvm -S -O3 -v ./dump/testscipt.ll -o ./dump/exec.ir

rnir:
	@echo ---------------------------------------------------------
	clang ./dump/testscipt.ll -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

cppcheck:
	cppcheck --enable=all --check-level=exhaustive --error-exitcode=1 src/ -i src/include/json.hpp -I src/ --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression

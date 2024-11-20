run:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./test/ -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec

orun:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./test/ -O3 -o ./dump/exec
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

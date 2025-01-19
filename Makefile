cstd:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	-./build/gigly ./std/ -o ./dump/std
	@echo ---------------------------------------------------------
	ld -r ./std/build/obj/*.o -o ./dump/std.o


run:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./hells/ -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec $(ARGS)

runr:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Release --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./hells/ -o ./dump/exec
	@echo ---------------------------------------------------------
	./dump/exec $(ARGS)

orun:
	@echo ---------------------------------------------------------
	cmake --build ./build --config Debug --target all -j 4 --
	@echo ---------------------------------------------------------
	./build/gigly ./hells/ -O3 -o ./dump/exec
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
	cppcheck --enable=all --check-level=exhaustive --error-exitcode=1 src/ -i src/include/ -I src/ --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=noExplicitConstructor --force

format:
	clang-format -i src/compiler/compiler.cpp src/compiler/compiler.hpp src/compiler/enviornment/enviornment.cpp src/compiler/enviornment/enviornment.hpp src/errors/errors.cpp src/errors/errors.hpp src/lexer/lexer.cpp src/lexer/lexer.hpp src/lexer/token.cpp src/lexer/token.hpp src/parser/AST/ast.cpp src/parser/AST/ast.hpp src/parser/parser.cpp src/parser/parser.hpp src/main.cpp

aout:
	./dump/a.out

exec:
	./dump/exec

setenv:
	export GC_STD_OBJ="/mnt/soham/soham_code/GigglyCode/dump/std.o"
	export GC_STD_DIR="/mnt/soham/soham_code/GigglyCode/std/src/"
	export GC_STD_IRGCMAP="/mnt/soham/soham_code/GigglyCode/std/build/ir_gc_map/"
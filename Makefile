
run:
	@echo ---------------------------------------------------------
	./build/gigly ./test/testscipt.gc -o ./dump/testscipt.ll

cppcheck:
	cppcheck --enable=all --check-level=exhaustive --error-exitcode=1 src/ -i src/include/json.hpp -I src/ --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression

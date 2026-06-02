# Test Big - Integrated Features Test

This test verifies all the major features of the GigglyCode compiler in a single integrated project, ensuring their compatibility and correct interaction.

## Verified Features:
1. **Module Imports:** Imports definitions from `modules/helper`.
2. **Generics:** Instantiates and uses a generic struct `helper.Box[int]`.
3. **OOP & Structs:** Defines `struct Point` with constructors and custom methods.
4. **Operator Overloading:** Overloads the `-` operator for `Point` via `__sub__`.
5. **Function Overloading:** Overloads `printVal` for `int` and `float` arguments.
6. **Enums:** Declares `enum Status` and invokes the built-in `.getName()` method.
7. **Control Flow:**
   - C-style `for` loops.
   - Standard `switch-case` statements with `other` fallback.
   - `while` loops with `notbreak` / `ifbreak` modifier blocks.
8. **Modifiers & Inference:**
   - Type inference using `:` followed by `=` and `auto`.
   - `const` and `volatile` variable declarations.
9. **Raw Arrays:** Allocation and subscription indexing of `raw_array(int, 3)`.

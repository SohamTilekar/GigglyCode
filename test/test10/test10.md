# Test 10 - Type Inference

This test verifies the type inference capabilities of the GigglyCode compiler.

## Verified Features:
- **Colon-equals type inference:** `x := expression;` where the compiler infers the type of `x` based on the right-hand side expression.
- **Auto keyword type inference:** `y: auto = expression;` where the `auto` keyword is used for type deduction.

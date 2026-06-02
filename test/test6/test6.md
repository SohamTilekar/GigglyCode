# Test 6 - Operator Overloading

This test verifies custom operator overloading for structs in GigglyCode.

## Verified Features:
- **Operator Overload definition:** `def __sub__(self: Point, _other: Point) -> float` inside the struct definition.
- **Infix Operator Resolution:** When `-` is used between two Point objects (`bottomLeft - topLeft`), the compiler maps it to the `__sub__` method call.

# Test 2 - Generic Structs

This test verifies the declaration and instantiation of generic structs in GigglyCode.

## Verified Features:
- **Generic Struct Decorator:** `@generic(T: Any)` applied to `struct ListNode` and `struct LinkedList`.
- **Parameterized Type References:** Referencing generic types such as `next: ListNode[T]` inside the class body.
- **Generic Instantiation:** Allocating a generic struct with explicit type arguments: `intList: LinkedList[int] = LinkedList(int);`.
- **Dynamic Allocation of Generic Nodes:** Instantiating nodes dynamically using the `new` operator: `new ListNode(T, value, nullptr)`.

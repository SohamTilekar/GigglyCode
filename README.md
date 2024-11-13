# GigglyCode

GigglyCode is an innovative programming language developed by SohamTilekar. Designed primarily as an educational tool for learning C++, GigglyCode aims to bridge the gap between different programming languages by supporting inline Python and C code in the future. This unique feature will make it an excellent choice for developers looking to leverage the strengths of multiple languages within a single project.

## Objective
The primary objective of GigglyCode is to facilitate interoperability between different programming languages. By allowing inline Python and C code in future versions, GigglyCode will enable developers to harness the power of these languages seamlessly within their projects. Future versions may include support for additional languages, further expanding its versatility.

## Features
### 1. Python-Inspired Syntax
GigglyCode's syntax is heavily influenced by Python, making it intuitive and easy to learn for Python developers. The key difference is the use of curly braces `{}` for block structuring instead of indentation.

### 2. Strong Typing and Type Inference
GigglyCode supports strong typing and type inference, ensuring type safety while reducing the verbosity of type declarations.

### 3. Advanced Control Structures
GigglyCode includes advanced control structures such as `if-else`, `while`, `for`, and `switch` statements, providing developers with powerful tools for controlling program flow.

### 4. Function and Struct Definitions
GigglyCode allows for the definition of functions and structs, enabling developers to organize their code effectively and create reusable components.

### 5. Comprehensive Standard Library
GigglyCode comes with a comprehensive standard library that includes a wide range of functions and utilities for common programming tasks.

### 6. Boolean Literals
GigglyCode supports boolean literals `True` and `False`.

### 7. Array Literals
GigglyCode supports array literals, allowing the creation of arrays with elements of the same type.

### 8. Index Expressions
GigglyCode supports index expressions, enabling array indexing and element access.

### 9. Infix Expressions
GigglyCode supports a variety of infix expressions for arithmetic, comparison, and logical operations.

### 10. Variable Declaration and Assignment
GigglyCode allows variable declaration with type annotations and supports variable assignment.

### 11. Import Statements
GigglyCode supports import statements to include other GigglyCode files.

### 12. While Loops
GigglyCode supports `while` loops for repeated execution of a block of code as long as a condition is true.

### 13. Break and Continue Statements
GigglyCode supports `break` and `continue` statements for controlling loop execution.

### 14. Function Parameters with Types
GigglyCode allows function parameters to have type annotations.

### 15. Closure Parameters
GigglyCode supports closure parameters in functions, allowing functions to capture variables from their surrounding scope.

### 16. Struct Methods
GigglyCode supports methods within structs, allowing functions to be associated with struct types.

### 17. Volatile Variables
GigglyCode supports the declaration of volatile variables, which can be modified by concurrent threads.

### 18. Metadata for Debugging
GigglyCode includes metadata for debugging, such as line numbers and column numbers for tokens and nodes.

### 19. Comprehensive Error Handling
GigglyCode has comprehensive error handling for syntax errors, type mismatches, and other compilation errors.

### 20. LLVM IR Generation
GigglyCode uses LLVM for intermediate representation (IR) generation, enabling optimizations and code generation for multiple target architectures.

### 21. Built-in Functions
GigglyCode includes built-in functions like `print` and `puts` for standard output.

### 22. Type Checking
GigglyCode performs type checking to ensure type safety and prevent type mismatches.

### 23. Function Return Types
GigglyCode supports function return types, allowing functions to specify the type of value they return.

### 24. Basic Block Management
GigglyCode manages basic blocks for functions, enabling structured control flow within functions.

### 25. Environment Management
GigglyCode manages environments for variable and function scopes, supporting nested scopes and closures.

### 26. Future Support for Inline Python and C Code
One of the standout features planned for GigglyCode is its support for inline Python and C code. This will allow developers to write Python and C code directly within their GigglyCode programs, leveraging the strengths of these languages where needed. However, please note that this feature is not currently supported and is planned for future versions.

## Syntax
Here's a quick example to illustrate what GigglyCode looks like:

```python
def fizzbuzz(start, stop, step) {
  for i in range(start, stop, step) {
    if i % 15 == 0 {
      print('fizzbuzz');
    } elif i % 5 == 0 {
      print('buzz');
    } elif i % 3 == 0 {
      print('fizz');
    } else {
      print(i);
    }
  }
}
```

### Function Definition
```python
def add(a: int, b: int) -> int {
  return a + b;
}
```

### Struct Definition
```python
struct Point {
  x: int;
  y: int;
}

def main() {
  let p = Point { x: 10, y: 20 };
  print(p.x, p.y);
}
```

## Getting Started
To get started with GigglyCode, follow these steps:

1. **Clone the Repository**: Clone the GigglyCode repository from GitHub.
   ```sh
   git clone https://github.com/yourusername/GigglyCode.git
   ```

2. **Build the Compiler**: Navigate to the project directory and build the compiler using CMake.
   ```sh
   cd GigglyCode
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Write Your First Program**: Create a new file with the `.gc` extension and write your first GigglyCode program.

4. **Compile and Run**: Use the GigglyCode compiler to compile and run your program.
   ```sh
   ./gigglycode your_program.gc -o output
   ./output
   ```

## Contributing
We welcome contributions from the community! If you'd like to contribute to GigglyCode, please follow these steps:

1. **Fork the Repository**: Fork the GigglyCode repository on GitHub.

2. **Create a Branch**: Create a new branch for your feature or bug fix.
   ```sh
   git checkout -b feature-name
   ```

3. **Make Changes**: Make your changes and commit them with a descriptive commit message.
   ```sh
   git commit -m "Add feature-name"
   ```

4. **Push to Your Fork**: Push your changes to your forked repository.
   ```sh
   git push origin feature-name
   ```

5. **Create a Pull Request**: Create a pull request on the main GigglyCode repository.

## License
GigglyCode is licensed under the MIT License. See the [LICENSE](LICENSE) file for more information.

## Contact
For any questions or inquiries, please contact SohamTilekar at [sohamtilekar233@gmail.com](mailto:sohamtilekar233@gmail.com).

Stay tuned for more updates as GigglyCode continues to evolve and improve! Thank you for your interest and support.

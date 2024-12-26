# 🎉 GigglyCode 🎉

[📖 Getting Started] | [📚 Learn] | [📝 Documentation] | [🤝 Contributing]

This is the main source code repository for **GigglyCode**. It contains the 🛠️ compiler, 📦 standard library, and 📄 documentation.

## ❓ Why GigglyCode?

- **🌉 Interoperability:** Bridges the gap between different 🖥️ programming languages by planning support for inline 🐍 Python and ⚙️ C code.
- **📚 Educational Tool:** Perfect for learning C++ concepts while enabling a 🧵 multi-language approach in a single project.
- **🚀 Performance:** Optimized for 🏃‍♂️ speed and 💾 memory efficiency, suitable for embedded systems, critical services, and integration with other languages.
- **🛡️ Reliability:** A robust type system and 🔗 ownership model ensure 🐞 bug-free memory and thread safety.
- **💡 Productivity:** Extensive 📖 documentation, excellent compiler 🛎️ diagnostics, and modern 🔧 tooling like 📦 package management, auto-formatters, linters, and IDE support.

## ✨ Features

- **📦 Variables:** Easy declaration and use.
- **🔁 Functions:** Reusable and maintainable code.
- **🏗️ Structs:** Custom data types for better 🗂️ organization.
- **🔀 Generic Structs & Functions:** Write reusable code across data types.
- **⚡ Function Overloading:** Define multiple ⚙️ functions with the same name but different parameters.
- **🆕 `new` Keyword:** Dynamic memory allocation made simple.
- **🤖 Type Inference:** Automatic 🕵️‍♂️ type detection for variables and expressions.
- **⚠️ Error Handling:** Graceful error management with 🧪 try-catch blocks.
- **📚 Modules:** Organize code into maintainable 🧩 modules.
- **🌉 Interoperability:** Planned support for inline 🐍 Python and ⚙️ C code.
- **🤝 Struct Methods:** Define 🛠️ methods that operate on struct instances.
- **🔁 Control Flow:** Flexible loops (`for`, `while`) with advanced controls (`break`, `continue`) targeting specific loop levels.
- **📥 Imports:** Effortless inclusion of 🏛️ standard libraries and user-defined 🧩 modules.

## 🛠️ Syntax Examples

### 🏗️ Structs and Methods
```python
# Define a struct to represent a rectangle
struct Rectangle {
  width: int;  # Width of the rectangle
  height: int;  # Height of the rectangle

  # Method to calculate the area of the rectangle
  def area(self: Rectangle) -> int {
    return self.width * self.height;
  }

  # Method to calculate the perimeter of the rectangle
  def perimeter(self: Rectangle) -> int {
    return 2 * (self.width + self.height);
  }
}
```

### 📦 Generics
```python
# Define a generic struct for a linked list node
@generic(T: Any)
struct ListNode {
  value: T;  # Generic value
  next: ListNode[T];  # Pointer to the next node

  # Constructor to initialize the node
  def __init__(self: ListNode[T], value: T, next: ListNode[T]) -> void {
    self.value = value;
    self.next = next;
  };
}

# Define a generic linked list
@generic(T: Any)
struct LinkedList {
  head: ListNode[T];  # Head node of the list

  # Constructor to initialize the linked list
  def __init__(self: LinkedList[T]) -> void {};

  # Method to add a new node to the list
  def add(self: LinkedList[T], value: T) -> void {
    newNode: ListNode[T] = new ListNode(value, nullptr);
    if (self.head == nullptr) {
      self.head = newNode;
    } else {
      current: ListNode[T] = self.head;
      while (current.next != nullptr) {
        current = current.next;
      }
      current.next = newNode;
    }
  }
}
```

### ⚡ Function Overloading
```python
# Define multiple overloaded functions for subtraction
def subtract(a: int, b: int) -> int {
  return a - b;
}

def subtract(a: float, b: float) -> float {
  return a - b;
}
```

### 🔁 Control Flow
```python
# Switch-case example
val: int = 2;
switch (val) {  # Switch-case is not yet implemented
  case 1:
    printf("Value is 1.");
    break;
  case 2:
    printf("Value is 2.");
    break;
  case 3:
    printf("Value is 3.");
    break;
  default:
    printf("Value is unknown.");
}

# While loop example
counter: int = 0;
while (counter < 5) {
    printf("Counter: %i", counter);
  counter = counter + 1;
}
```

### 🆕 Dynamic Memory Allocation
```python
# Dynamic memory allocation for an array
arr: int[] = new int[5]; # Allocate an array like this is not yet implemented
for (i in range(0, 5)) {
  arr[i] = i * 10;
}
for (i in range(0, 5)) {
    printf("Array element %i : %i", i, arr[i]);
}
```

### 📥 Imports
```python
# Import example with usage
import "modules/math";

struct Calculator {
  def add(self: Calculator, a: int, b: int) -> int {
    return math.add(a, b);  # Use the imported math module
  }

  def subtract(self: Calculator, a: int, b: int) -> int {
    return math.subtract(a, b);  # Use the imported math module
  }
}

calc: Calculator = Calculator();
printf("Addition result: %i", calc.add(10, 5));
printf("Subtraction result: %i", calc.subtract(10, 5));
```

### 🏗️ Complete Program Example
```python
# Main function to demonstrate all features
def main() {
  # Struct and method usage
  rect: Rectangle = Rectangle(10, 20);  # Initialize rectangle with width 10 and height 20
  printf("Rectangle area: %i", rect.area());  # Calculate and print area
  printf("Rectangle perimeter: %i", rect.perimeter());  # Calculate and print perimeter

  # Generic struct usage
  intList: LinkedList[int] = LinkedList(int);  # Linked list with integers
  intList.add(10);
  intList.add(20);
  intList.add(30);
  current: ListNode[int] = intList.head;
  while (current != nullptr) {
      printf("List node value: %i", current.value);
    current = current.next;
  }

  # Function overloading usage
  printf("Subtract integers: %i", subtract(10, 5));  # Calls the int version
  printf("Subtract floats: %f", subtract(10.5, 5.5));  # Calls the float version

  arr: array[int] = [0, 10, 20, 30, 40]; # Allocate an array with elements [[0, 10, 20, 30, 40]]

  # Dynamic memory allocation example
  arr: int = new int[5]; # Allocate an array like this is not yet implemented
  for (i in range(0, 5)) {
    arr[i] = i * 10;
  }
  for (i in range(0, 5)) {
      printf("Array element %i : %i", i, arr[i]);
  }

  # Control flow demonstration
  val: int = 2;
  switch (val) { # Switch-case is not yet implemented
    case 1:
      print("Value is 1.");
      break;
    case 2:
      print("Value is 2.");
      break;
    case 3:
      print("Value is 3.");
      break;
    default:
      print("Value is unknown.");
  }

  # Import and usage of a module
  import "modules/math";
  calc: Calculator = Calculator();
  printf("Addition result: %i", calc.add(10, 5));
  printf("Subtraction result: %i", calc.subtract(10, 5));
}
```

## 🛫 Getting Started

1. **🧑‍💻 Clone the Repository**
   ```sh
   git clone https://github.com/yourusername/GigglyCode.git
   ```

2. **🛠️ Build the Compiler**
    _Before this Make sure that You Have compiled & installed llvm & yaml_
   ```sh
   cd GigglyCode
   mkdir build
   cd build
   cmake ../src
   cd ..
   cmake --build ./build --config Release --target all -j 4 -- <!--or--> cmake --build ./build --config Debug --target all -j 4 --
   # The  Cmake will Output the `./build/giggly` which is the main exec of the giggly code
   ```

3. **📜 Write Your First Program** Create a `.gc` file and write your 🛠️ GigglyCode program.

4. **🚀 Compile and Run** Compile an entire project:

   ```sh
   ./build/giggly ./project_dir/ -O2 -o executable_path
   ```

   - 🗂️ `project_dir`: The root 📁 containing the `src` folder where `main.gc` resides.
   - ⚙️ `-O2`: Optimization level (e.g., `O1`, `O2`, `O3`, `Ofast`).
   - 🛤️ `executable_path`: Specifies the path for the compiled executable.

### 📁 Example Project Structure

```html
📁 project/
  📂 src/
    📄 main.gc
    📂 modules/
      📄 io.gc
```

## 🙌 Contributing

We 💖 contributions! Here’s how you can get involved:

- You know How to do this If you dont that means you are a beginner so do not try to contribute in OpenSource Project untill you have 1 year+ experience.

## 📜 License

GigglyCode is distributed under the 📝 MIT license. See the [LICENSE](LICENSE) file for details.

## 📧 Contact

<!--For 💬 questions or feedback, reach out to **Soham Tilekar** at [sohamtilekar233@gmail.com](mailto:sohamtilekar233@gmail.com).-->

Thank you for your 🎉 interest in GigglyCode! Stay tuned for updates as we continue to 🚀 improve.


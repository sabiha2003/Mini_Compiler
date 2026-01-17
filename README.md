 Mini-Compiler Project: Technical Documentation

## 1. Project Overview

This project is a **Deterministic Source-to-Source Compiler (Transpiler)** developed in C++. It translates a custom mathematical DSL (Domain Specific Language) into executable **C code**. It features a full compilation pipeline, including lexical analysis, syntax parsing, an internal interpreter, and a target code generator.

---

## 2. The Compilation Pipeline (Logic-to-Code)

### **Phase 1: Lexical Analysis (The Scanner)**

* **In the Code:** `Lexer::tokenize()`
* **Process:** It scans the raw source text and groups characters into **Tokens**. It uses pattern recognition to identify `ID` (variables), `NUMBER` (integers), and `RESERVED_WORDS` (like `print`).
* **Outcome:** Strips whitespace and provides a clean token stream for the parser.

### **Phase 2: Syntax Analysis (The Parser)**

* **In the Code:** `Parser::parse()`
* **Process:** Implements a **Top-Down Recursive Descent** algorithm. It verifies the "Grammar" of the code and enforces **Operator Precedence** (BODMAS).
* **Outcome:** Constructs an **Abstract Syntax Tree (AST)**, a hierarchical map of the program logic.

### **Phase 3: Semantic Analysis & Execution**

* **In the Code:** `SymbolTable` class & `eval_node()` function.
* **Memory Management:** Uses a `std::map<string, int>` (Symbol Table) to track variable states (e.g., `{a: 100, b: 2}`).
* **Execution:** The `eval_node` function performs a recursive walk of the AST to calculate immediate results for the console.

### **Phase 4: Code Generation (The Backend)**

* **In the Code:** `codegen()` function.
* **Process:** Transpiles the AST into standard C syntax. It injects necessary headers (`#include <stdio.h>`) and wraps the logic into a `main()` function.
* **Outcome:** Generates `output.c`, which is portable and ready for compilation via `GCC`.

---

## 3. Data Flow Example (`logic.txt`)

**Input:**

```cpp
a = 100;
b = 2;
res = (a + 50) * b / (10 - 5);
print(res);

```

**Execution Logic:**

1. **Symbol Table:** Stores `a=100`, `b=2`.
2. **AST Walk:** Calculates `(100+50) * 2 / 5`  `60`.
3. **Result:** Prints `60` and generates the C source.

---

## 4. Future Roadmap

* **Control Flow:** Adding keywords for `if-else` and `while` loops.
* **Type Expansion:** Support for `float` and `string` data types.
* **Optimization:** Implementing **Constant Folding** to pre-calculate values during compilation.

---

## 5. Conclusion

This compiler successfully demonstrates how high-level human logic is transformed into structured machine instructions. By maintaining a modular separation between the **Front-end** (Scanning/Parsing) and the **Back-end** (Execution/Generation), the system remains scalable for complex language features.


# Leaf

A fast, minimal, dynamically-typed programming language built in modern C++.

Leaf is designed to be clean, readable, and easy to extend.  
The current implementation includes a fully working lexer with support for functions, loops, arrays, conditions, operators, comments, and token generation.

---

# Features

- Clean Python-inspired syntax
- Dynamically typed
- Built in modern C++17
- Fully working lexer
- Arrays and collections
- Functions and returns
- Control flow
- Boolean logic
- Comments
- Easy to extend architecture
- Tokenized output for debugging

---

# Example

```leaf
// Variables
x = 10
name = "Leaf"

// Arrays
nums = [1, 2, 3]

// Function
fun add(a, b) {
    return a + b
}

// Conditions
if x > 5 {
    print("big")
} elseif x == 5 {
    print("equal")
} else {
    print("small")
}

// Loops
while x > 0 {
    x = x - 1
}

for i -> nums {
    print(i)
}
```

---

# Syntax Guide

## Variables

```leaf
x = 42
name = "hello"
pi = 3.14
active = true
nothing = null
```

---

## Strings

```leaf
message = "hello world"
```

Strings currently support basic quoted text.

---

## Arrays

```leaf
nums = [1, 2, 3]
```

---

## Functions

```leaf
fun add(a, b) {
    return a + b
}
```

---

## Conditionals

```leaf
if x > 10 {
    print("large")
} elseif x > 5 {
    print("medium")
} else {
    print("small")
}
```

---

## While Loops

```leaf
while x > 0 {
    x = x - 1
}
```

---

## For Loops

```leaf
for item -> items {
    print(item)
}
```

---

## Boolean Logic

```leaf
if a == b && x > 0 {
    print("valid")
}

if !done || retry {
    print("continue")
}
```

---

# Operators

## Arithmetic

| Operator | Meaning |
|----------|----------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Modulo |

---

## Comparison

| Operator | Meaning |
|----------|----------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

---

## Logical

| Operator | Meaning |
|----------|----------|
| `&&` | Logical AND |
| `||` | Logical OR |
| `!` | Logical NOT |

---

# Comments

Single-line comments:

```leaf
// this is a comment
```

---

# Current Token Support

## Literals

- Numbers
- Strings
- Identifiers

---

## Keywords

- `fun`
- `return`
- `if`
- `elseif`
- `else`
- `while`
- `for`
- `break`
- `skip`
- `true`
- `false`
- `null`
- `print`

---

## Delimiters

- `(`
- `)`
- `{`
- `}`
- `[`
- `]`
- `,`
- `.`

---

# Project Structure

```txt
leaf/
├── src/
│   ├── main.cpp
│   ├── lexer.cpp
│   └── lexer.h
├── README.md
└── Makefile
```

---

# Building

## Linux / macOS

```bash
g++ -std=c++17 -Wall -Wextra -O2 \
src/main.cpp \
src/lexer.cpp \
-o leaf
```

Run:

```bash
./leaf
```

---

## Windows (MinGW)

```bash
g++ -std=c++17 -Wall -Wextra -O2 ^
src/main.cpp ^
src/lexer.cpp ^
-o leaf.exe
```

Run:

```bash
leaf.exe
```

---

# Example Output

```txt
=== LEAF TOKENS ===
line 1    IDENTIFIER    "x"
line 1    ASSIGN        "="
line 1    NUMBER        "10"
...
```

---

# Lexer Capabilities

The lexer currently supports:

- Integer numbers
- Floating point numbers
- Strings
- Keywords
- Identifiers
- Operators
- Logical expressions
- Arrays
- Braces and delimiters
- Comments
- Line tracking
- Error reporting

---

# Roadmap

## Completed

- [x] Lexer
- [x] Token system
- [x] Keywords
- [x] Operators
- [x] Arrays
- [x] Comments
- [x] Control flow tokens

---

## In Progress

- [ ] Parser
- [ ] AST generation
- [ ] Expression precedence
- [ ] Runtime system

---

## Planned

- [ ] Interpreter
- [ ] Standard library
- [ ] File imports
- [ ] Error recovery
- [ ] REPL
- [ ] Bytecode VM
- [ ] Garbage collector
- [ ] Package manager

---

# Design Goals

Leaf aims to be:

- Minimal
- Readable
- Fast
- Easy to modify
- Beginner friendly
- Powerful enough for scripting
- Simple to embed into C++ applications

---

# Why "Leaf"?

Because every large system starts small.

Leaf is intended to grow naturally into a complete scripting ecosystem.

---

# License

MIT License

---

# Author

Built in modern C++.
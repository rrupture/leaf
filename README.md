# 🍃 Leaf

A fast, minimal, dynamically-typed programming language built in modern C++.

Leaf is designed to be clean, readable, and easy to extend.

---

## Install

**Windows** — paste this in PowerShell:

```powershell
irm https://raw.githubusercontent.com/777rupture/leaf/main/install.ps1 | iex
```

Then open a new terminal and you're good to go.

**Linux / macOS** — clone and build manually:

```bash
git clone https://github.com/777rupture/leaf.git
cd leaf
g++ -std=c++17 -Wall -O2 src/main.cpp src/lexer.cpp -o leaf
sudo mv leaf /usr/local/bin/leaf
```

---

## Usage

```powershell
leaf -create hello.leaf   # create a new file with a starter template
leaf -run hello.leaf      # run a Leaf file
leaf -tokens hello.leaf   # debug: print token output
leaf -help                # show all commands
```

---

## Example

```leaf
// Variables
x = 10
name = "Leaf"

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

// While loop
while x > 0 {
    x = x - 1
}

// For loop
for i -> nums {
    print(i)
}
```

---

## Syntax

### Variables
```leaf
x = 42
name = "hello"
pi = 3.14
active = true
nothing = null
```

### Functions
```leaf
fun add(a, b) {
    return a + b
}
```

### Conditionals
```leaf
if x > 10 {
    print("large")
} elseif x > 5 {
    print("medium")
} else {
    print("small")
}
```

### Loops
```leaf
while x > 0 {
    x = x - 1
}

for item -> items {
    print(item)
}
```

### Arrays
```leaf
nums = [1, 2, 3]
```

### Boolean Logic
```leaf
if a == b && x > 0 {
    print("valid")
}

if !done || retry {
    print("continue")
}
```

### Comments
```leaf
// this is a comment
```

---

## Operators

| Type | Operators |
|------|-----------|
| Arithmetic | `+` `-` `*` `/` `%` |
| Comparison | `==` `!=` `<` `>` `<=` `>=` |
| Logical | `&&` `\|\|` `!` |

---

## Building from Source

```bash
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp src/lexer.cpp -o leaf
```

---

## Roadmap

- [x] Lexer
- [x] Token system
- [x] CLI (`-create`, `-run`, `-tokens`)
- [ ] Parser
- [ ] AST
- [ ] Interpreter
- [ ] Standard library
- [ ] REPL
- [ ] Package manager

---

## Why "Leaf"?

Because every large system starts small.

---

## License

MIT
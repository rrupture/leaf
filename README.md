## Author

[rrupture](https://github.com/rrupture)

---


# 🍃 Leaf

A fast, minimal, dynamically-typed scripting language built in modern C++.

---

## Install

**Windows** — paste this in PowerShell:

```powershell
irm https://raw.githubusercontent.com/rrupture/leaf/main/install.ps1 | iex
```

Open a new terminal and you're good to go.

**Linux / macOS:**

```bash
git clone https://github.com/rrupture/leaf.git
cd leaf
g++ -std=c++17 -Wall -O2 src/main.cpp src/lexer.cpp -o leaf
sudo mv leaf /usr/local/bin/leaf
```

---

## Usage

```powershell
leaf -create hello.leaf   # create a new file
leaf -run    hello.leaf   # run a Leaf file
leaf -repl                # interactive mode
leaf -tokens hello.leaf   # debug token output
leaf -update              # how to update
leaf -help                # show all commands
```

---

## Example

```leaf
// variables
name = "Leaf"
x = 10

// functions
fun square(n) {
    return n * n
}

// arrays
nums = [1, 2, 3, 4, 5]
for n -> nums {
    result = square(n)
    if result > 10 {
        print("big: <result>")
    } else {
        print("small: <result>")
    }
}

// while loop
count = 3
while count > 0 {
    print("countdown: <count>")
    count = count - 1
}

// string methods
msg = "  hello leaf  "
print(msg.trim().upper())

// math
print(math.sqrt(144))

// range
for i -> range(5) {
    print(i)
}

// file system
fs.write("out.txt", "hello from leaf")
content = fs.read("out.txt")
print(content)

// user input
name = input("your name: ")
print("hi <name>!")
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

result = add(3, 4)
print(result)
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

### While Loop
```leaf
while x > 0 {
    x = x - 1
}
```

### For Loop
```leaf
for item -> items {
    print(item)
}

// with range
for i -> range(10) {
    print(i)
}
```

### Arrays
```leaf
nums = [1, 2, 3]
nums.push(4)
nums.pop()
nums.clear()
print(nums.len())
print(nums.has(2))
```

### String Methods
```leaf
name.upper()
name.lower()
name.trim()
name.split(" ")
name.replace("a", "b")
name.contains("hello")
name.startswith("he")
name.endswith("lo")
name.len()
```

### String Interpolation
```leaf
name = "Alex"
print("hello <name>!")
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

## Standard Library

### Math
```leaf
math.sqrt(x)
math.floor(x)
math.ceil(x)
math.abs(x)
math.pow(x, y)
math.sin(x)
math.cos(x)
math.tan(x)
math.log(x)
math.random()
math.pi()
```

### File System
```leaf
fs.read("file.leaf")
fs.write("file.leaf", content)
fs.exists("file.leaf")
fs.delete("file.leaf")
```

### Built-ins
```leaf
print(x)
input("prompt: ")
range(10)
range(2, 10)
range(0, 10, 2)
len(list)
str(x)
int(x)
float(x)
bool(x)
```

---

## Operators

| Type | Operators |
|------|-----------|
| Arithmetic | `+` `-` `*` `/` `%` |
| Comparison | `==` `!=` `<` `>` `<=` `>=` |
| Logical | `&&` `\|\|` `!` |

---

## VS Code Extension

Syntax highlighting for `.leaf` files:

1. Download from [leaf-vscode](https://github.com/rrupture/leaf-vscode)
2. Drop the folder into `C:\Users\yourname\.vscode\extensions\leaf-language-1.0.0\`
3. Restart VS Code

---

## Roadmap

- [x] Lexer
- [x] Token system
- [x] CLI (`-create`, `-run`, `-tokens`, `-repl`)
- [x] Interpreter
- [x] Functions
- [x] Arrays + for loops
- [x] Standard library (math, strings, fs)
- [x] REPL
- [x] VS Code extension
- [ ] Package manager
- [ ] Error recovery
- [ ] Bytecode VM

---

## License

MIT
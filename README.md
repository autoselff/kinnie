# Kinnie
## Setup

**Important! Remember, to use the built-in graphics library, you must have the SDL and SDL_ttf library installed. Also, make sure you have gcc and g++ compilers installed. Kinnie does not support the underscore character in variable, function and struct names.**

Debian
```bash
sudo apt install cmake libsdl2-dev libsdl2-ttf-dev gcc g++
```

Arch
```bash
sudo pacman -S cmake sdl2 sdl2_ttf gcc
```

Copy repository
```bash
gh repo clone autoselff/kinnie
```
```bash
cd kinnie
```

Compile kinnie (CMake — recommended)
```bash
cmake -B build
cmake --build build
```

Add to PATH
```bash
sudo cmake --install build
```

## Running
```bash
kinnie main.kn
```

### Command-line Flags

- `--version` — Display version information
- `--compile` — Compile only, do not run the generated executable
- `--remcpp` — Remove the generated C++ file after compilation
- `--stime` — Show compilation time statistics for each stage (tokenization, includes, parsing, codegen, C++ compilation)

## Language Syntax

### Variables

Defining variables:
```kinnie
var x = 10
var name = "Hello"
var pi = 3.14
```

### Arrays

Arrays store multiple values in a single variable. Arrays can contain numbers, strings, and other arrays.

Creating arrays:
```kinnie
var numbers = [10, 20, 30, 40]
var mixed = [100, 200, "text", 400]
var empty = []
```

Accessing array elements (0-indexed):
```kinnie
var first = numbers[0]   // 10
var second = numbers[1]  // 20
var text = mixed[2]      // "text"
```

Modifying array elements:
```kinnie
numbers[0] = 15
mixed[2] = "new text"
```

Array in loops:
```kinnie
var tab = [10, 11, 12, 13, 4235]
var l = len(tab)
rep l {
    out "{tab[l]}\n"
}
```

#### Multi-dimensional Arrays

Arrays can be nested arbitrarily to create multi-dimensional structures. All nested arrays use the same `_KnTable` type internally.

**Creating nested arrays:**

1D array:
```kinnie
var numbers = [10, 20, 30]
```

2D array (matrix):
```kinnie
var matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
```

3D array:
```kinnie
var cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
```

**Accessing elements:**
```kinnie
var val1 = matrix[0][1]      // 2
var val2 = cube[0][1][0]     // 3
matrix[1][2] = 99
cube[0][0][1] = 42
```

**Getting array dimensions:**
```kinnie
var t = [[[1, 2], [3, 4], [5, 6], [7, 8]], [[5, 6], [7, 8]], [[5, 6], [7, 8]]]
var l = len(t)
var ll = len(t[0])
var lll = len(t[0][0])

out "{l}\n"     // 3
out "{ll}\n"    // 4
out "{lll}\n"   // 2
```

**Iterating through multi-dimensional arrays:**

2D iteration:
```kinnie
fun display2D(t) {
    var i = 0
    var rows = len(t)
    rep rows {
        var j = 0
        var cols = len(t[i])
        rep cols {
            out "{t[i][j]} "
            j = j + 1
        }
        out "\n"
        i = i + 1
    }
}
```

3D iteration:
```kinnie
fun process3D(t) {
    var i = 0
    var len0 = len(t)
    rep len0 {
        var j = 0
        var len1 = len(t[i])
        rep len1 {
            var k = 0
            var len2 = len(t[i][j])
            rep len2 {
                t[i][j][k] = t[i][j][k] * 2
                k = k + 1
            }
            j = j + 1
        }
        i = i + 1
    }
}
```

**Key points:**
- Nest arrays using bracket notation: `[[], []]` for 2D, `[[[]]]` for 3D, etc.
- Use `len(array)` to get the size of the first dimension
- Use `len(array[i])`, `len(array[i][j])`, etc. for nested dimensions
- Arrays are passed to functions by reference (modifications affect the original)

Practical example with game data:
```kinnie
var player = [400, 300, 60, 50, 100, 200, "player"]
var playerX = player[0]      // 400
var playerY = player[1]      // 300
var playerSize = player[2]   // 60
var playerName = player[6]   // "player"
```

### Arithmetic Operators

- `+` addition
- `-` subtraction  
- `*` multiplication
- `/` division

```kinnie
var sum = 5 + 3
var diff = 10 - 2
var prod = 4 * 5
var div = 20 / 4
```

### Math Functions

| Function | Description |
|---|---|
| `sin(x)` | sine of x (radians) |
| `cos(x)` | cosine of x (radians) |
| `abs(x)` | absolute value |
| `sqrt(x)` | square root of x |
| `exp(x)` | e raised to the power x |
| `log(x)` | natural logarithm of x |
| `log10(x)` | base-10 logarithm of x |
| `pow(a, b)` | a raised to the power b |
| `min(a, b)` | smaller of two values |
| `max(a, b)` | larger of two values |
| `mod(a, b)` | floating-point modulo |
| `lerp(a, b, t)` | linear interpolation: `a + (b-a) * t` |
| `distance(x1, y1, x2, y2)` | Euclidean distance between two points |
| `sizeof(x)` | size in bytes of a value |

```kinnie
var angle = 1.57
var s = sin(angle)       // ~1.0
var c = cos(angle)       // ~0.0
var v = abs(-5)          // 5
var lo = min(3, 7)       // 3
var hi = max(3, 7)       // 7
var r = mod(10.5, 3)     // 1.5
var m = lerp(0, 100, 0.25)       // 25
var d = distance(0, 0, 3, 4)     // 5
```

### Comparison Operators

- `==` equal
- `!=` not equal
- `>` greater than
- `<` less than
- `>=` greater or equal
- `<=` less or equal

```kinnie
if x == 5 {
    out "x is 5"
}

if y > 10 {
    out "y is greater than 10"
}
```

### Conditional Statements

```kinnie
if condition {
    out "Condition is true"
}
else {
    out "Condition is false"
}
```

### Loops

```kinnie
var i = 5
rep i {
    out "{i}," // 0,1,2,3,
    if i == 3 {
        stop
    }
}
```

The `rep` loop iterates N times. The counter variable must be defined beforehand.

### Output

```kinnie
out 42           // Prints: 42
out "Hello"      // Prints: Hello
out "x = {x}"    // Print with variable substitution
```

Variable interpolation in strings: `{variable_name}`

### Functions

Defining:
```kinnie
fun greet(name) {
    out "Hello, {name}!"
}
```

Calling:
```kinnie
greet("World")

var name = "Joe"
greet(name)
```

Returning values:
```kinnie
fun add(a, b) {
    ret a + b
}

fun main() {
    var x = 10
    var result = add(x, 3)
}
```

Arrays passed to functions are always passed as originals (by reference). Modifications inside the function affect the original array:
```kinnie
fun foo(t) {
    var l = len(t) - 1
    rep l {
        var temp = l + 1
        t[l] = t[l] + t[temp]
    }
}

fun sho(t) {
    var l = len(t)
    rep l {
        out "{t[l]}\n"
    }
}

fun main() {
    var tab = [100, 200, 300, 400, 500]
    foo(tab)
    sho(tab)
}

```

The compiler detects array parameters automatically — no special syntax needed.

### Structs

Defining a struct with fields and methods:
```kinnie
str Player {
    var x = 0
    var y = 0
    var hp = 100

    fun move(dx, dy) {
        x = x + dx
        y = y + dy
    }

    fun print() {
        out "pos: {x}, {y} hp: {hp}\n"
    }
}
```

Creating an instance:
```kinnie
str Player p
```

Accessing and modifying fields:
```kinnie
p.x = 400
p.y = 300
var health = p.hp
```

Calling methods:
```kinnie
p.move(10, 5)
p.print()
```

Passing struct instances to functions (passed by reference — modifications affect the original):
```kinnie
fun damage(p, amount) {
    p.hp = p.hp - amount
}

fun main() {
    str Player p
    p.hp = 100
    damage(p, 25)
    out "{p.hp}\n"   // 75
}
```

The compiler detects struct parameters automatically by looking for dot-notation usage inside the function body.

Limits: maximum 32 structs, 32 fields per struct, 16 methods per struct.

### Including Files

```kinnie
add "lib.kn"
```

## SDL2 Graphics Functions

### Window Initialization

```kinnie
createWindow(800, 600, "My Game")
```

Parameters: `(width, height, title)`

### Clear Screen

```kinnie
clearScreen(r, g, b)
```

Parameters: RGB values (0-255)


### Drawing Functions

#### `drawPixel(x, y, r, g, b)`
Draws a single pixel.

#### `drawSquare(x, y, size, r, g, b)`
Draws a filled square.
```kinnie
drawSquare(100, 100, 50, 255, 0, 0)  // red square at (100,100), size 50
```

#### `drawCircle(x, y, radius, r, g, b)`
Draws a filled circle using the midpoint circle algorithm.
```kinnie
drawCircle(200, 150, 40, 0, 255, 0)  // green circle at (200,150), radius 40
```

### Text

```kinnie
setFont("path/to/font.ttf")
```

#### `drawText(x, y, text, size, r, g, b)`
Draws text using system fonts. Requires SDL2_ttf library.
```kinnie
drawText(50, 50, "Hello World", 24, 255, 255, 255)  // white text at (50,50), size 24
```

### Random Functions

#### `random(min, max)` → number


## Keyboard Input

### `keyPressed(key)` → 0 or 1
Checks if a key was just pressed.

### `keyDown(key)` → 0 or 1
Checks if a key is currently held down.

```kinnie
if keyPressed("space") {
    out "Space pressed!"
}

if keyDown("left") {
    out "Left arrow held"
}
```

Supported keys: `"left"`, `"right"`, `"up"`, `"down"`, `"space"`, `"a"`, `"b"`, etc.

## Frame Timing

### `deltaTime`
A global variable automatically available in SDL2 programs. It holds the time in seconds elapsed since the last frame.

Use it to make movement frame-rate independent:
```kinnie
var speed = 200
playerX = playerX + speed * deltaTime
```

`deltaTime` is updated at the start of every frame, before any user code runs. It requires `createWindow` to be used (SDL2 mode only).

## Common Issues

**Problem:** "SDL2 not found"
- **Solution:** Install SDL2:
  - Debian: `sudo apt install libsdl2-dev`
  - Arch: `sudo pacman -S sdl2`

**Problem:** `drawText()` doesn't render text
- **Solution:** Install SDL2_ttf library:
  - Debian: `sudo apt install libsdl2-ttf-dev`
  - Arch: `sudo pacman -S sdl2_ttf`

**Problem:** Function doesn't return value
- **Solution:** Use `ret value` instead of `return`

**Problem:** Objects not drawing properly
- **Solution:** Ensure function parameters are not modified by `rep` loops. Use temporary variables.

## Limitations

- Maximum 8192 tokens per file (after includes)
- Maximum 64 functions
- Maximum 16 parameters per function
- Maximum 32 characters in a name
- Maximum 128 characters in a string literal
- All numeric values are floating point (`double`)
- Maximum 32 structs, 32 fields per struct, 16 methods per struct
- Structs cannot be returned from functions or initialized with `= expr`
- Arrays can be nested arbitrarily (unlimited dimensions)
- No built-in array methods like `push()` or `pop()` — use `len()` for size and direct indexing `[]` for access

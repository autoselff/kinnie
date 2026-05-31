# Kinnie
<div align="center">
    <img width="300" height="300" alt="logo_kinnie" src="https://github.com/user-attachments/assets/9c421472-a058-42c5-8621-bdf183207f69" />
</div>
    
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
- `--keepcpp` — Keep the generated C++ file after compilation
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

Printing arrays:
```kinnie
var tab = [1, 2, "Hello", [3, 4]]
out "{tab}\n"       // [1, 2, "Hello", [3, 4]]
out "{tab[3]}\n"    // [3, 4]
```

#### Array Methods

##### `.add(value)`
Appends a value to the end of an array. Accepts numbers, strings, and nested arrays.
```kinnie
var tab = [1, 2, 3]
tab.add(4)         // [1, 2, 3, 4]
tab.add("hello")   // [1, 2, 3, 4, "hello"]
tab.add([5, 6])    // [1, 2, 3, 4, "hello", [5, 6]]
```

##### `.remove(index)`
Removes the element at the given index.
```kinnie
var tab = [1, 2, 3, 4]
tab.remove(0)              // [2, 3, 4]
tab.remove(len(tab) - 1)  // removes last element
```

##### `.clear()`
Removes all elements from the array, making it empty.
```kinnie
var tab = [1, 2, 3, 4]
tab.clear()   // tab is now []
```

All methods work on nested arrays too:
```kinnie
var tab = [1, 2, [3, 4]]
tab[2].remove(0)   // tab is now [1, 2, [4]]
tab[2].add(99)     // tab is now [1, 2, [4, 99]]
tab[2].clear()     // tab is now [1, 2, []]
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

### Compound Assignment Operators

- `+=` add and assign
- `-=` subtract and assign
- `*=` multiply and assign
- `/=` divide and assign

```kinnie
var x = 100
x += 50    // x = 150
x -= 30    // x = 120
x *= 2     // x = 240
x /= 4     // x = 60
```

### Increment / Decrement

```kinnie
var i = 0
i++    // i = 1
i--    // i = 0
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
| `clamp(x, min, max)` | clamp value between min and max |
| `round(x)` | round to nearest integer |
| `floor(x)` | round down to nearest integer |
| `ceil(x)` | round up to nearest integer |
| `mod(a, b)` | floating-point modulo |
| `lerp(a, b, t)` | linear interpolation: `a + (b-a) * t` |
| `distance(x1, y1, x2, y2)` | Euclidean distance between two points |

```kinnie
var angle = 1.57
var s = sin(angle)              // ~1.0
var c = cos(angle)              // ~0.0
var v = abs(-5)                 // 5
var lo = min(3, 7)              // 3
var hi = max(3, 7)              // 7
var clamped = clamp(5, 0, 3)    // 3
var rounded = round(3.7)        // 4.0
var floored = floor(3.7)        // 3.0
var ceiled = ceil(3.2)          // 4.0
var r = mod(10.5, 3)            // 1.5
var m = lerp(0, 100, 0.25)      // 25
var d = distance(0, 0, 3, 4)    // 5
delay(0.5)                      // pause for 0.5 seconds
```

### Other Functions
| Function | Description |
|---|---|
| `random(min, max)` | returns a random number between min and max |
| `sizeof(x)` | size in bytes of a value |
| `delay(seconds)` | pause execution for specified seconds |


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

### Logical Operators

- `and` — logical AND
- `or` — logical OR
- `not` — logical NOT

```kinnie
if a > 0 and b > 0 {
    out "both positive\n"
}

if a > 10 or b > 10 {
    out "at least one is big\n"
}

if not a == 99 {
    out "a is not 99\n"
}
```

### Conditional Statements

```kinnie
if score >= 90 {
    out "A\n"
} else if score >= 75 {
    out "B\n"
} else if score >= 60 {
    out "C\n"
} else {
    out "F\n"
}
```

### Loops

Count loop — iterates N times:
```kinnie
var i = 5
rep i {
    out "{i}\n"
}
```

While loop — repeats while condition is true:
```kinnie
var x = 0
rep x < 10 {
    out "{x}\n"
    x++
}
```

Use `stop` to break out of a loop early:
```kinnie
var i = 0
rep 20 {
    if i == 5 {
        stop
    }
    out "{i}\n"
    i++
}
```

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

Arrays passed to functions are always passed as originals (by reference). Modifications inside the function affect the original array.

**Array parameters must be declared with `[]` syntax:**
```kinnie
fun foo(t[]) {
    var l = len(t) - 1
    rep l {
        var temp = l + 1
        t[l] = t[l] + t[temp]
    }
}

fun display(t[]) {
    var l = len(t)
    rep l {
        out "{t[l]}\n"
    }
}

fun main() {
    var tab = [100, 200, 300, 400, 500]
    foo(tab)
    display(tab)
}
```

When declaring a function parameter as an array, use `paramName[]` syntax. This tells the compiler that the parameter should receive an array reference:
```kinnie
fun clearAndDisplay(arr[]) {
    arr.clear()
    out "Array cleared\n"
}

fun addElements(nums[], value) {
    nums.add(value)
    nums.add(value * 2)
}
```

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
Player p        // str keyword is optional
```

Or with `str` keyword (equivalent):
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
    Player p
    p.hp = 100
    damage(p, 25)
    out "{p.hp}\n"   // 75
}
```

```kinnie
str Objects {
    var objects = [1, 2, 3]

    fun show() {
        var l = len(objects)
        var i = 0
        rep l {
            out "{objects[i]}\n"
            i = i + 1
        }
    }

    fun multiply() {
        var l = len(objects)
        var i = 0
        rep l {
            objects[i] = objects[i] * 10
            i = i + 1
        }
    }
}

fun addd(a, b, c, d) {
    ret a + b + c + d
}

fun show(x) {
    out "{x}\n"
}

fun main() {
    Objects objs
    objs.show()
    objs.multiply()
    objs.show()

    out "{addd(1, 2, 3, 6546456456456)}\n"
    show("Hello, World!")
    show(34534)

}
```

The compiler detects struct parameters automatically by looking for dot-notation usage inside the function body.

**Note:** The `str` keyword is optional when creating struct instances. `Player p` and `str Player p` are equivalent.

Limits: maximum 32 structs, 32 fields per struct, 16 methods per struct.

### Including Files

```kinnie
add "lib.kn"
```

## SDL2 Graphics Functions

### Window Initialization

```kinnie
createWindow(800, 600, "Kinnie")
```

Parameters: `(width, height, title)`

### Clear Screen

```kinnie
fun main() {
    createWindow(800, 600, "Kinnie")
    setFont("/usr/share/fonts/Adwaita/AdwaitaMono-Regular.ttf")

    var i = 0
    rep 1 {
        clearScreen(0, 0, 0
        drawText(180, 270, "{i}", 50, 255, 0, 0)
        i = i + 1
        delay(0.5)
    }
}

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

#### `drawRectangle(x, y, sizeX, sizeY, r, g, b)`
Draws a filled rectangle with specified width and height.
```kinnie
drawRectangle(100, 100, 100, 50, 255, 0, 0)  // red rectangle at (100,100), 100x50
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
- Array methods: `.add(value)` appends, `.remove(index)` removes by index, `.clear()` empties the array; all work on nested arrays
- Array parameters must be declared with `[]` syntax: `fun foo(arr[]) { ... }`

## NOTE
**AI was used to create this project.** To learn and test Claude Code, the Haiku 4.5 model was responsible for creating documentation, comments, splitting the code into files, and implementing functionalities such as arrays and data structures. I always include a note in projects where AI was used.

# Kinnietype
A simple and minimalist programming language made for game development (Compiles to C++)

## Compilation

```bash
gcc -o kinnie kinnie.c
```

## Running
Important! Remember, to use the built-in graphics library, you must have the SDL library downloaded.
```bash
sudo apt install libsdl2-dev
```

```bash
./kinnie main.kn -c
```

The `-c` flag compiles the code to C++ and runs the program.

## Language Syntax

### Variables

Defining variables:
```kinnie
var x = 10
var name = "Hello"
var pi = 3.14
```

### Arrays

Arrays store multiple values in a single variable. Arrays can contain numbers and strings.

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
```kinnie
var tab = [10, 11, 12, 13, 4235]
var len = tab.len
rep len {
    out "{tab[len]}\n"
}
```

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
| `min(a, b)` | smaller of two values |
| `max(a, b)` | larger of two values |
| `mod(a, b)` | floating-point modulo |
| `lerp(a, b, t)` | linear interpolation: `a + (b-a) * t` |
| `distance(x1, y1, x2, y2)` | Euclidean distance between two points |

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
    out "{i}," // 0,1,2,3,4,
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
fun doubleFirst(arr) {
    arr[0] = arr[0] * 2
}

fun main() {
    var nums = [5, 10, 15]
    doubleFirst(nums)
    out "{nums[0]}\n"
}
```

The compiler detects array parameters automatically — no special syntax needed.

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

### Draw Pixel

```kinnie
drawPixel(x, y, r, g, b)
```

### Drawing Functions

#### `drawPixel(x, y, r, g, b)`
Draws a single pixel.

#### `drawSquare(x, y, size, r, g, b)`
Draws a filled square.
```kinnie
drawSquare(100, 100, 50, 255, 0, 0)  // red square at (100,100), size 50
```

#### `drawCircle(x, y, radius, r, g, b)`
Draws a circle outline using the midpoint circle algorithm.
```kinnie
drawCircle(200, 150, 40, 0, 255, 0)  // green circle at (200,150), radius 40
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
- **Solution:** Install SDL2: `sudo apt install libsdl2-dev`

**Problem:** `rep` loop not working
- **Solution:** Loop counter variable must be defined first: `var i = 0; rep 10 { ... }`

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
- No structs/objects

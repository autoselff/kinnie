# Kinnietype
## Setup

Important! Remember, to use the built-in graphics library, you must have the SDL library downloaded. Also, make sure you have gcc and g++ compilers installed.

Debian
```bash
sudo apt install libsdl2-dev gcc g++
```

Arch
```bash
sudo pacman -S sdl2 gcc
```

Copy repository
```bash
gh repo clone autoselff/kinnie
```
```bash
cd kinnie
```

Compile kinnie
```bash
gcc -o kinnie kinnie.c
```
or
```bash
gcc -o kinnie kinnie.c $(sdl2-config --cflags --libs)
```

Add to PATH
```bash
sudo cp kinnie /usr/local/bin/ && sudo chmod +x /usr/local/bin/kinnie
```

## Running
```bash
kinnie main.kn
```

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
- `mod()` modulo

```kinnie
var sum = 5 + 3
var diff = 10 - 2
var prod = 4 * 5
var div = 20 / 4
var rem = mod(16, 5)
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
    var l = t.len - 1
    rep l {
        var temp = l + 1
        t[l] = t[l] + t[temp]
    }
}

fun sho(t) {
    var l = t.len
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


### Geometry and Collision Functions

#### `distance(x1, y1, x2, y2)` → number
Distance squared between two points.
```kinnie
var dist = distance(0, 0, 3, 4)
```

#### `isColliding(x1, y1, size1, x2, y2, size2)` → 0 or 1
Checks collision between two circles.
```kinnie
var collision = isColliding(10, 10, 5, 20, 20, 5)
if collision {
    out "Collision!"
}
```

### Drawing Functions

#### `drawPixel(x, y, r, g, b)`
Draws a single pixel.

#### `drawSquare(x, y, size, r, g, b)`
Draws a filled square.
```kinnie
drawSquare(100, 100, 50, 255, 0, 0)  // Red square
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

**Problem:** Function doesn't return value
- **Solution:** Use `ret value` instead of `return`

**Problem:** Objects not drawing properly
- **Solution:** Ensure function parameters are not modified by `rep` loops. Use temporary variables.

## Limitations

- Maximum 8192 tokens per file
- Maximum 512 functions
- Maximum 64 parameters per function
- Maximum 256 characters in variable name
- Maximum 1024 characters in string
- All numeric values are floating point numbers (double)
- No structs/objects (open for implementation)

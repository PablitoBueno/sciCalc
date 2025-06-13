# SciCalc: Scientific Calculator Embedded in ATmega328P

For clarity and adaptation to Tinkercad, a simulation model is available at https://www.tinkercad.com/things/15OlVMyJR4i-scicalc, which uses different connections, commands, and a simplified code compared to the GitHub version, but offers the same functionality and capabilities.

## 1. Abstract
- **Goal:** Design of a scientific calculator embedded in ATmega328P capable of evaluating generic arithmetic expressions and solving polynomial equations up to degree 5.
- **Contributions:**
  1. Custom recursive descent parser, balancing simplicity and memory constraints.
  2. Horner’s method for polynomial evaluation with O(n) complexity.
  3. Hybrid sampling and bisection strategy for real root finding in degree ≥3 polynomials.
  4. Footprint optimizations: PROGMEM storage, inline functions, and fixed-size buffers.
- **Main Results:** Accuracy of 1e-6 for simple roots, average processing time <50 ms for degree-5 polynomials, binary <25 kB.

## 2. Introduction
Embedded calculation solutions are essential for education and low-cost portable devices. Many existing methods are too heavy for low-memory microcontrollers. SciCalc addresses this limitation by providing a lean solution for expressions and univariate polynomials with real roots, focusing on simplicity and numerical precision.

## 3. Hardware Architecture
The project uses the ATmega328P microcontroller, a 16×2 LCD in 4-bit mode, and a 4×4 matrix keypad for data input with multi-tap support. UART serial is used for additional output. The system is optimized for minimal memory and pin usage.

### Components Used
| Quantity | Component              |
|----------|------------------------|
| 1        | Arduino Uno (ATmega328P)|
| 1        | 16×2 LCD (4-bit mode)  |
| 1        | 4×4 Matrix Keypad      |
| Various  | Jumper wires & breadboard |

### LCD Connection Map
| LCD Pin | Arduino Pin | Description              |
|---------|-------------|--------------------------|
| RS      | A1          | Command register         |
| RW      | GND         | Write mode (fixed)       |
| E       | PD2         | Enable                   |
| D4      | A2          | Data (4-bit mode)        |
| D5      | A3          | Data                     |
| D6      | A4          | Data                     |
| D7      | A5          | Data                     |

### Keypad Connection Map
| Line / Column | Arduino Pin  |
|---------------|--------------|
| R1            | D7           |
| R2            | D8           |
| R3            | D9           |
| R4            | D10          |
| C1            | D3           |
| C2            | D4           |
| C3            | D5           |
| C4            | D6           |

### Key Mapping
| Key | Functions                     | Description                             |
|-----|-------------------------------|-----------------------------------------|
| 0   | `1`                           | Digit 1                                 |
| 1   | `2`                           | Digit 2                                 |
| 2   | `3`                           | Digit 3                                 |
| ... | ...                           | ...                                     |
| 13  | `+`, `-`, `*`, `/`, `=`, `S`, `^`, `(`, `)` | Math operators            |
| 14  | `LFT`                         | Cursor left                             |
| 15  | `RGT`                         | Cursor right                            |

### Multi-Tap Input
Keys with multiple characters use multi-tap input (like old cell phones). Repeated key presses within 2.5s cycle through characters.

## 4. Tools & Technologies
- **IDE & Compiler:** Arduino IDE 1.8.x, AVR-GCC 7.3  
- **Language:** Lightweight C/C++  
- **Features:** PROGMEM strings, fixed buffers, inline functions  
- **Libraries:** AVR libc, `<avr/pgmspace.h>`

## 5. Programming Aspects
### 5.1 Validation & Parsing
```cpp
bool validateInput(const char* input);
float evaluateExpression(const char *expr);
```

### 5.2 Polynomial Evaluation
```cpp
float evaluatePoly(const float poly[], int degree, float x);
```

### 5.3 Polynomial Equation Solving
```cpp
int solvePolynomialEquation(const char *eq, float roots[], char var);
```

### 5.4 Real Root Finding
```cpp
int findRealPolynomialRoots(const float poly[], int degree, float rootsFound[]);
```

### 5.5 Output Formatting
```cpp
void formatResultFloat(float res, char *resultStr, int size);
```

## 6. Mathematical Background
- **Horner:** O(n) evaluation  
- **Bisection:** Guaranteed convergence, tolerance 1e-6  
- **Sampling:** Based on Cauchy bound for root intervals  
- **Error Handling:** `fabs` comparison, duplicate filtering  
- **Square Root:** `S25` means √25

## 7. Tests, Performance & Use Cases
### 7.1 Performance
- **SRAM:** 285 bytes (13%)  
- **Flash:** 13,194 bytes (40%)  
- **Time (deg. 5):** ~2.9 ms  
- **Precision:** mean error 4e-7; max 9e-7  
- **Display Limitation:** LCD shows max 2 roots; all sent via UART

### 7.2 Examples

#### Arithmetic: `2+3*(4-1)` → `11.00`
```cpp
float r = evaluateExpression("2+3*(4-1)");
```

#### Linear: `2x+4=0` → `-2.00`
```cpp
int n = solvePolynomialEquation("2x+4=0", roots, 'x');
```

#### Quadratic: `x^2-5x+6=0` → `{2.00, 3.00}`
```cpp
int n = solvePolynomialEquation("x^2-5x+6=0", roots, 'x');
```

#### Cubic: `x^3-6x^2+11x-6=0` → `{1.00, 2.00, 3.00}`
```cpp
int n = solvePolynomialEquation("x^3-6x^2+11x-6=0", roots, 'x');
```

## 8. Discussion
- **Limitations:** Single-variable, real roots only. No complex roots or transcendental functions.
- **Alternatives Not Used:** Newton-Raphson (needs derivative), ASTs (too heavy).
- **Future Work:** Hybrid root solvers, CORDIC, graphical displays, symbolic algebra, i18n.

## 9. Conclusion
- **Summary:** SciCalc offers efficient parsing and polynomial solving on constrained systems.
- **Impact:** Suitable for education, didactic kits, and embedded devices.

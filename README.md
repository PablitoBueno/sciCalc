# Arduino Equation Solver

This project allows an Arduino to evaluate general mathematical expressions and solve equations containing an incognito variable (x, y, or z). It uses a 16x2 LCD display for output and a 4x4 matrix keypad for input.

## Features

- **General Expression Evaluation:**  
  Evaluate arithmetic expressions like `5+3*2` or compute square roots (using `S16` for âˆš16).

- **Equation Solving:**  
  Detects and solves equations with an incognito variable, such as `x+2=5` or polynomial equations like `2x^2+3x-5=0`.

- **Interactive Keypad Input:**  
  A 4x4 matrix keypad with multi-tap functionality lets you enter numbers, operators, and variables.

- **LCD Display:**  
  A 16x2 LCD shows the project title on the first line and the current input on the second line, along with a blinking cursor.

## Hardware Requirements & Wiring

### Components
- Arduino Board (Uno, Mega, etc.)
- 16x2 LCD Display
- 4x4 Matrix Keypad
- Jumper Wires

### LCD Connections

| LCD Pin | Arduino Pin | Description                         |
|---------|-------------|-------------------------------------|
| RS      | A1          | Register Select                     |
| RW      | A0          | Read/Write (tied low for write)     |
| E       | PD2         | Enable signal                       |
| D4      | A2          | Data line (4-bit mode)              |
| D5      | A3          | Data line                           |
| D6      | A4          | Data line                           |
| D7      | A5          | Data line                           |

*Note: The LCD contrast is set by connecting the contrast adjustment node to GND (no potentiometer or resistors needed).*

### Keypad Connections

| Keypad Pin | Arduino Pin |
|------------|-------------|
| Row 1      | 7           |
| Row 2      | 8           |
| Row 3      | 9           |
| Row 4      | 10          |
| Column 1   | 3           |
| Column 2   | 4           |
| Column 3   | 5           |
| Column 4   | 6           |

## Key Mapping

The 4x4 keypad keys are assigned as follows:

| **Key Index** | **Options**                                                   | **Function**                                |
|---------------|---------------------------------------------------------------|---------------------------------------------|
| 0             | `1`                                                           | Digit 1                                     |
| 1             | `2`                                                           | Digit 2                                     |
| 2             | `3`                                                           | Digit 3                                     |
| 3             | `4`                                                           | Digit 4                                     |
| 4             | `5`                                                           | Digit 5                                     |
| 5             | `6`                                                           | Digit 6                                     |
| 6             | `7`                                                           | Digit 7                                     |
| 7             | `8`                                                           | Digit 8                                     |
| 8             | `9`                                                           | Digit 9                                     |
| 9             | `0`                                                           | Digit 0                                     |
| 10            | `BK`                                                          | Backspace (delete previous character)       |
| 11            | `ENT`                                                         | Enter (evaluate the input)                  |
| 12            | `x`, `y`, `z`                                                 | Incognito variable selection                |
| 13            | `+`, `-`, `*`, `/`, `(`, `)`, `.`, `=`, `S`, `^`               | Operators and symbols selection             |
| 14            | `LFT`                                                         | Move cursor left                            |
| 15            | `RGT`                                                         | Move cursor right                           |

### Multi-Tap Input
Keys with multiple options (indices 12 and 13) use multi-tap: pressing the key repeatedly (within 2.5 seconds) cycles through the available characters. The selected character is then inserted into your input.

## Usage Instructions

1. **Power Up:**  
   When you power the Arduino, the LCD displays "SciCalc-Equation" on the first line and a blinking cursor on the second line.

2. **Input an Expression or Equation:**  
   - **Entering Numbers & Operators:** Use numeric keys (0â€“9) and key 13 (for operators like `+`, `-`, `*`, `/`, etc.). For example, to type `5+3*2`, press:
     - Key 4 for `"5"`.
     - Key 13 to select `"+"`.
     - Key 2 for `"3"`.
     - Key 13 again to select `"*"` (if needed).
     - Key 1 for `"2"`.
   - **Entering an Incognito Variable:** For an equation like `x+2=5`, press key 12 to select the variable (cycle to choose `x`, `y`, or `z`), then continue with numbers and operators.
   - **Editing:**  
     - Use key 10 (`BK`) to delete a character.
     - Use keys 14 (`LFT`) and 15 (`RGT`) to move the cursor.

3. **Evaluate the Input:**  
   Press key 11 (`ENT`) to evaluate the expression or solve the equation. The result will replace the current input on the LCD.

## Code Overview

- **LCD Functions:**  
  Control the LCD by sending commands and data directly through port manipulation.

- **Keypad Functions:**  
  Initialize and scan the 4x4 matrix keypad, including handling multi-tap input.

- **Expression Parsing:**  
  A recursive descent parser evaluates arithmetic expressions while maintaining operator precedence.

- **Equation Solving:**  
  The code checks for the presence of an incognito variable and, if found, extracts coefficients to solve linear or polynomial equations.

- **Input Management:**  
  The input is stored in a buffer that supports inserting, deleting, and moving the cursor.

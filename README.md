
# Arduino Calculator with LCD and Keypad

This is a simple calculator project implemented using an Arduino, a 4x4 keypad, and a 16x2 LCD. The project allows users to perform basic mathematical operations such as addition, subtraction, multiplication, and division, as well as advanced operations like square root, trigonometric functions, and logarithms. The calculator is controlled via the keypad, and results are displayed on the LCD.

## Project Overview

The main features of this project include:

- **4x4 Keypad**: Used for inputting numbers, operations, and controlling the flow of the program.
- **16x2 LCD**: Used to display instructions, input values, operations, and results.
- **Operations Supported**: Basic arithmetic operations (+, -, *, /), square root, trigonometric functions, and logarithms.
- **Menu System**: A simple menu system to select between different operations.

## Components Used

- **Arduino** (any compatible board such as Uno, Nano, etc.)
- **16x2 LCD** (with parallel connection)
- **4x4 Keypad**
- **Breadboard and jumper wires**

## Circuit Connections

### 1. **LCD Pin Connections**:

#### For a **Parallel LCD Connection** (16x2 LCD):

- **RS** to A1
- **RW** to A0
- **Enable** to pin 2
- **D4** to A2
- **D5** to A3
- **D6** to A4
- **D7** to A5
- **VSS** (Ground) to GND (Arduino Ground)
- **VDD** (Power) to 5V (Arduino 5V Pin)
- **V0** (Contrast) directly connected to 5V (no potentiometer used)

#### Important Connections:

- **VSS** (Ground) on both the LCD and Keypad modules should be connected to the **Arduino Ground (GND)** pin. This ensures that both modules share a common ground.
- **VDD** (Power) of both the LCD and Keypad should be connected to the **5V** pin on the Arduino board. This provides power to the modules.

### 2. **Keypad Pin Connections**:

- **Rows**: Pin 7, 8, 9, 10
- **Columns**: Pin 3, 4, 5, 6
- **VSS** (Ground) of Keypad to GND (Arduino Ground)
- **VDD** (Power) of Keypad to 5V (Arduino 5V Pin)

### 3. **Power Connections**:

- **VSS** (Ground) on both the LCD and Keypad modules should be connected to the **Arduino Ground (GND)** pin. This ensures that both modules share a common ground.
- **VDD** (Power) of both the LCD and Keypad should be connected to the **5V** pin on the Arduino to supply them with proper voltage.

### 4. **Other Important Connections**:

- **Arduino Ground (GND)**: Ensure that all components that require ground (LCD, Keypad) are connected to the Arduino **GND** pin.
- **Arduino 5V**: The power connections (VDD) for the LCD and Keypad should be linked to the **5V** pin on the Arduino to supply them with proper voltage.

## Code Explanation

### Libraries Used

1. **LiquidCrystal**: This library is used to interface the LCD with the Arduino.
2. **Keypad**: This library handles the keypad interface, making it easier to read the input from the 4x4 keypad.
3. **Math**: The math library is used for advanced functions such as square root, trigonometric functions, and logarithms.

### Variables

- **LCD Pin Definitions**: Defines the pins used for the LCD.
- **Keypad Configuration**: Defines the layout of the keypad and assigns the respective Arduino pins for rows and columns.
- **Operations**: A list of operations supported by the calculator (e.g., addition, multiplication, trigonometric functions).
- **Flags**: Several flags are used to track the state of the calculation (e.g., `firstOperandConfirmed`, `operationConfirmed`, `showingResult`).

### Setup Function

In the `setup()` function, the LCD is initialized, and the program displays a welcome message ("Calculator") followed by a prompt asking the user to enter a value.

### Main Loop

1. **Key Press Detection**: The main loop continuously checks for key presses from the keypad. When a key is pressed, the program processes the input accordingly.
   
2. **Input Handling**: If the user enters numbers or decimal points, the program builds a string representing the current input. The user can also press the minus key (`-`) to toggle the sign of the number.

3. **Operation Handling**: After entering the first operand (number), the user can choose an operation (e.g., addition, multiplication) by scrolling through the menu using the `L` and `R` keys.

4. **Calculation**: When the user presses `=`, the program checks whether both operands and the operation have been selected. It then performs the calculation and displays the result on the LCD.

5. **Unary Operations**: For unary operations (e.g., square root, sine, cosine), the program immediately calculates and displays the result once the first operand is confirmed.

6. **Error Handling**: If the user tries to divide by zero, an error message is displayed.

7. **Reset State**: After displaying the result, the program enters a state where the user can start a new calculation by pressing `=` again.

### Keypad Controls

- **Number Keys** (`0-9`): Enter numbers.
- **Decimal Point** (`.`): Enter a decimal point.
- **Minus Key** (`-`): Toggle the negative sign of the number.
- **E**: Delete the last character or return to the previous step in the calculation flow.
- **L**: Navigate left through the list of operations.
- **R**: Navigate right through the list of operations.
- **=**: Confirm the input, perform the operation, and display the result.

### Operations

The calculator supports a variety of operations:

1. **Basic Arithmetic**: Addition (`+`), subtraction (`-`), multiplication (`*`), and division (`/`).
2. **Advanced Functions**:
   - Square root (`sqrt`)
   - Trigonometric functions: sine (`sin`), cosine (`cos`), tangent (`tan`), inverse sine (`asin`), inverse cosine (`acos`), inverse tangent (`atan`), hyperbolic sine (`sinh`), hyperbolic cosine (`cosh`), hyperbolic tangent (`tanh`).
   - Logarithmic functions: natural logarithm (`log`), base 10 logarithm (`log10`), base 2 logarithm (`log2`).
   - Exponential (`exp`)
   - Power (`pow`)
   - Factorial (`fact`)
   - Cube root (`cbrt`)
   - Modulo operation (`%`)

### Mathematical Functions

- The functions are implemented using standard mathematical formulas and functions provided by the Arduino `math.h` library.
- The factorial function (`fact`) is implemented iteratively, and checks are made for non-negative inputs.

### State Management

The state of the calculator is managed using several flags:

- `firstOperandConfirmed`: Indicates if the first operand has been entered.
- `operationConfirmed`: Indicates if an operation has been selected.
- `showingResult`: Indicates if the result of the calculation is currently being displayed.
- `input`: Holds the current input (number or result).
- `isNegative` and `hasDecimalPoint`: Track the state of the current number's sign and decimal point.

### Error Handling

- **Division by Zero**: If the user tries to divide by zero, an error message is displayed.
- **Input Validation**: The program ensures that decimal points are only allowed once per operand, and it handles backspace correctly.

### How to Use

1. Upon powering up the Arduino, the LCD will display "Calculator" followed by a prompt to enter a value.
2. Enter the first operand (number) using the keypad.
3. Navigate through the operation menu with the `L` and `R` keys and select the desired operation.
4. Enter the second operand (if needed) and press `=` to calculate the result.
5. The result will be displayed on the LCD.
6. To reset the calculator, press `=` again after the result is shown, and it will clear the input.

## Conclusion

This Arduino calculator is a simple yet effective way to perform both basic and advanced mathematical operations using an LCD and a keypad. It demonstrates the use of an interactive user interface with hardware components such as an LCD and keypad, making it a useful project for learning about user input handling, mathematical calculations, and state management on the Arduino platform.

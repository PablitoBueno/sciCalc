#include <LiquidCrystal.h>
#include <Keypad.h>
#include <math.h>

// LCD pin definitions
const int rs = A1;
const int rw = A0;
const int enable = 2;
const int d4 = A2;
const int d5 = A3;
const int d6 = A4;
const int d7 = A5;

// Initialize the LCD
LiquidCrystal lcd(rs, rw, enable, d4, d5, d6, d7);

// 4x4 keypad definitions
const byte ROW_NUM = 4; // 4 rows
const byte COLUMN_NUM = 4; // 4 columns

// Keypad mapping
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','0','E','='},
  {'-','.','L','R'}  // 'L' for left in the menu, 'R' for right in the menu
};

// Pins connected to the keypad
byte pin_rows[ROW_NUM] = {7, 8, 9, 10};  // Rows connected to pins 7 to 10
byte pin_column[COLUMN_NUM] = {3, 4, 5, 6};  // Columns connected to pins 3 to 6

// Create the keypad object
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Variables to store input and result
String input = "";  // User input
float operand1 = 0.0;
float operand2 = 0.0;
int menuIndex = 0;  // Index for the operation menu
String menuOperations[] = {"+", "-", "*", "/", "sqrt", "sin", "cos", "tan", "log", "pow", "fact", "exp", "log10", "log2", "asin", "acos", "atan", "sinh", "cosh", "tanh", "cbrt", "%"};  // Available operations
bool operationConfirmed = false;
bool firstOperandConfirmed = false;
bool hasDecimalPoint = false;  // Flag to control decimal point
bool isNegative = false;  // Flag to control if the number is negative
bool showingResult = false;  // Flag to indicate if the result is being shown

void setup() {
  lcd.begin(16, 2);
  lcd.print("Calculator");
  
  delay(2000);
  lcd.clear();
  lcd.print("Enter a value");
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (showingResult) {
      if (key == '=') {
        resetState();
        lcd.clear();
        lcd.print("Enter a value");
      }
      return;
    }

    if ((key >= '0' && key <= '9') || key == '.') {
      if (key == '.' && hasDecimalPoint) {
        input.remove(input.length() - 1);
        hasDecimalPoint = false;
      } else if (key == '.') {
        hasDecimalPoint = true;
      }
      input += key;
      updateLCD(firstOperandConfirmed ? "Operand 2:" : "Operand 1:");
    }
    else if (key == '-') {
      isNegative = !isNegative;
      updateLCD(firstOperandConfirmed ? "Operand 2:" : "Operand 1:");
    }
    else if (key == 'E') {
      if (input.length() > 0) {
        input.remove(input.length() - 1);
        if (input.indexOf('.') == -1) hasDecimalPoint = false;
        updateLCD(firstOperandConfirmed ? "Operand 2:" : "Operand 1:");
      } else {
        if (firstOperandConfirmed && operationConfirmed) {
          operationConfirmed = false;
          updateMenu();
        } else if (firstOperandConfirmed) {
          firstOperandConfirmed = false;
          updateLCD("Operand 1:");
        } else {
          lcd.clear();
          lcd.print("Enter a value");
        }
      }
    }
    else if (key == '=') {
      if (!firstOperandConfirmed) {
        operand1 = isNegative ? -input.toFloat() : input.toFloat();
        input = "";
        isNegative = false;
        hasDecimalPoint = false;
        firstOperandConfirmed = true;
        updateMenu();
      } else if (!operationConfirmed) {
        operationConfirmed = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Operation:");
        lcd.setCursor(0, 1);
        lcd.print(menuOperations[menuIndex]);
        if (isUnaryOperation(menuOperations[menuIndex])) {
          input = String(calculate());
          updateLCD("Result:");
          showingResult = true;
        } else {
          input = "";
          updateLCD("Operand 2:");
        }
      } else {
        operand2 = isNegative ? -input.toFloat() : input.toFloat();
        input = String(calculate());
        updateLCD("Result:");
        showingResult = true;
      }
    }
    else if (key == 'L') {
      menuIndex = (menuIndex - 1 + 22) % 22;  // Updated to the number of operations
      updateMenu();
    }
    else if (key == 'R') {
      menuIndex = (menuIndex + 1) % 22;  // Updated to the number of operations
      updateMenu();
    }
  }
}

void updateLCD(String label) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);
  lcd.setCursor(isNegative ? 1 : 0, 1);
  if (isNegative) lcd.print("-");
  lcd.print(input);
}

void updateMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select:");
  lcd.setCursor(0, 1);
  lcd.print(menuOperations[menuIndex]);
}

float calculate() {
  float res = 0.0;
  if (menuOperations[menuIndex] == "+") {
    res = operand1 + operand2;
  } else if (menuOperations[menuIndex] == "-") {
    res = operand1 - operand2;
  } else if (menuOperations[menuIndex] == "*") {
    res = operand1 * operand2;
  } else if (menuOperations[menuIndex] == "/") {
    if (operand2 != 0) {
      res = operand1 / operand2;
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error: Div 0");
      delay(2000);
    }
  } else if (menuOperations[menuIndex] == "%") {
    res = fmod(operand1, operand2);  // Modulo operation
  } else if (menuOperations[menuIndex] == "sqrt") {
    res = sqrt(operand1);
  } else if (menuOperations[menuIndex] == "sin") {
    res = sin(operand1);
  } else if (menuOperations[menuIndex] == "cos") {
    res = cos(operand1);
  } else if (menuOperations[menuIndex] == "tan") {
    res = tan(operand1);
  } else if (menuOperations[menuIndex] == "log") {
    res = log(operand1);
  } else if (menuOperations[menuIndex] == "pow") {
    res = pow(operand1, operand2);
  } else if (menuOperations[menuIndex] == "fact") {
    res = factorial(operand1);
  } else if (menuOperations[menuIndex] == "exp") {
    res = exp(operand1);
  } else if (menuOperations[menuIndex] == "log10") {
    res = log10(operand1);
  } else if (menuOperations[menuIndex] == "log2") {
    res = log(operand1) / log(2);  // Logarithm base 2
  } else if (menuOperations[menuIndex] == "asin") {
    res = asin(operand1);
  } else if (menuOperations[menuIndex] == "acos") {
    res = acos(operand1);
  } else if (menuOperations[menuIndex] == "atan") {
    res = atan(operand1);
  } else if (menuOperations[menuIndex] == "sinh") {
    res = sinh(operand1);
  } else if (menuOperations[menuIndex] == "cosh") {
    res = cosh(operand1);
  } else if (menuOperations[menuIndex] == "tanh") {
    res = tanh(operand1);
  } else if (menuOperations[menuIndex] == "cbrt") {
    res = cbrt(operand1);
  }
  return res;
}

float factorial(float n) {
  if (n <= 1) return 1;
  float result = 1;
  for (int i = 2; i <= n; i++) {
    result *= i;
  }
  return result;
}

bool isUnaryOperation(String op) {
  return op == "sqrt" || op == "sin" || op == "cos" || op == "tan" || op == "log" || op == "fact" || op == "exp" || op == "log10" || op == "log2" || op == "asin" || op == "acos" || op == "atan" || op == "sinh" || op == "cosh" || op == "tanh" || op == "cbrt";
}

void resetState() {
  firstOperandConfirmed = false;
  operationConfirmed = false;
  input = "";
  isNegative = false;
  hasDecimalPoint = false;
  showingResult = false;
}

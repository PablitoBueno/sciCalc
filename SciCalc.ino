#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function prototypes
void formatResultFloat(float res, char *resultStr, int size); // Format a float into a string (scientific notation if needed)
void processSidePoly(const char *side, float polyArr[], char var); // Process one side of the equation to extract coefficients
float evaluateExpression(const char *expr); // Evaluate an arithmetic expression and return its value
int solvePolynomialEquation(const char *eq, float roots[], char var); // Solve a polynomial equation and return its roots
void sendRootsToSerial(float root1, float root2); // Envia as duas raízes via Serial, se conectado ao PC

// Constants and definitions
#define SQRT_SYMBOL "S"             // Symbol used for square root
#define ERROR_MSG "ERROR"           // Message displayed on error
#define NOSOL_MSG "No Solution"     // Message displayed when there is no solution
#define INF_SOL_MSG "Inf. Solutions"// Message for infinite solutions
#define MAX_POLY_DEGREE 5           // Maximum supported polynomial degree
#define TOLERANCE 1e-6              // Tolerance for value comparisons

// Input buffer and state variables for the interface
char eqInputBuffer[17] = "";        // Buffer for the entered equation (max 16 characters)
uint8_t eqInputLength = 0;          // Current length of the equation in the buffer
uint8_t cursorPosition = 0;         // Current cursor position in the buffer
bool messageDisplayed = false;      // Flag indicating a non-editable message is being shown

// LCD pin definitions
#define LCD_RS_PIN  A1
#define LCD_RW_PIN  A0
#define LCD_E_PIN   2
#define LCD_D4_PIN  A2
#define LCD_D5_PIN  A3
#define LCD_D6_PIN  A4
#define LCD_D7_PIN  A5
#define LCD_RS_MASK (1<<PC1)
#define LCD_RW_MASK (1<<PC0)
#define LCD_D4_MASK (1<<PC2)
#define LCD_D5_MASK (1<<PC3)
#define LCD_D6_MASK (1<<PC4)
#define LCD_D7_MASK (1<<PC5)
#define LCD_E_MASK  (1<<PD2)

// Sends a nibble (4 bits) to the LCD
static inline void lcdSendNibble(uint8_t nibble) { 
  PORTC = (PORTC & ~((1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5))) | ((nibble & 0x0F) << 2); // Prepare 4 bits for sending
  PORTD |= LCD_E_MASK;  // Set the LCD enable signal
  delayMicroseconds(1); 
  PORTD &= ~LCD_E_MASK; // Clear the enable signal
  delayMicroseconds(80);
}

// Sends a command to the LCD
static inline void lcdCommand(uint8_t cmd) { 
  PORTC &= ~((1 << PC1)|(1 << PC0)); // RS and RW set to 0 for command mode
  lcdSendNibble(cmd >> 4);           // Send upper nibble
  lcdSendNibble(cmd & 0x0F);          // Send lower nibble
  delay(2);
}

// Sends data (a character) to the LCD
static inline void lcdWriteData(uint8_t data) { 
  PORTC = (PORTC & ~((1 << PC1)|(1 << PC0))) | (1 << PC1); // RS=1 for data, RW=0
  lcdSendNibble(data >> 4);           // Send upper nibble
  lcdSendNibble(data & 0x0F);          // Send lower nibble
  delay(2);
}

// Initializes the LCD and sets it to 4-bit mode
void lcdInit() { 
  DDRC |= (1 << PC0)|(1 << PC1)|(1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5); // Set PORTC as output
  DDRD |= (1 << PD2);                // Set PD2 as output for the E pin
  PORTC &= ~((1 << PC0)|(1 << PC1)|(1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5)); // Clear PORTC
  PORTD &= ~LCD_E_MASK;              // Clear the E signal
  delay(50);
  lcdCommand(0x01);  // Clear the display
  delay(2);
  lcdSendNibble(0x03); delay(5); 
  lcdSendNibble(0x03); delay(5);
  lcdSendNibble(0x03); delayMicroseconds(150); 
  lcdSendNibble(0x02);  // Set to 4-bit mode
  lcdCommand(0x28);     // Configure for 2 lines and 5x8 dots font
  lcdCommand(0x08);     // Turn off display for setup
  lcdCommand(0x01);     // Clear display again
  delay(2);
  lcdCommand(0x06);     // Set cursor to increment mode
  lcdCommand(0x0C);     // Turn on display and hide cursor
}

// Clears the LCD display
void lcdClear() { 
  lcdCommand(0x01); 
  delay(2); 
}

// Sets the cursor position on the LCD
void lcdSetCursor(uint8_t col, uint8_t row) { 
  const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54}; // Starting addresses for each line
  lcdCommand(0x80 | (col + row_offsets[row])); // Position the cursor according to column and row
}

// Prints a string to the LCD
void lcdPrint(const char *str) { 
  while (*str) 
    lcdWriteData(*str++); // Send each character to the display
}

// Matrix keypad configuration: define row and column pins
const byte rowPins[4] = {7, 8, 9, 10};   // Row pins
const byte colPins[4] = {3, 4, 5, 6};      // Column pins

// Initializes the matrix keypad with rows as outputs and columns with pull-ups
void keypadInit() { 
  for (int i = 0; i < 4; i++) { 
    pinMode(rowPins[i], OUTPUT);  
    digitalWrite(rowPins[i], HIGH); // Set rows HIGH initially
  } 
  for (int i = 0; i < 4; i++) { 
    pinMode(colPins[i], INPUT_PULLUP); // Set columns as input with pull-up resistor
  } 
}

// Returns the index of the pressed key or -1 if none is pressed
int getKeyIndex() { 
  for (int r = 0; r < 4; r++) { 
    digitalWrite(rowPins[r], LOW); // Activate one row at a time
    uint8_t portD = PIND; 
    for (int c = 0; c < 4; c++) { 
      if (!(portD & (1 << colPins[c]))) { // Detect key press in the column
        delay(50); 
        while (!(PIND & (1 << colPins[c]))); // Wait for key release
        digitalWrite(rowPins[r], HIGH); 
        return r * 4 + c; // Calculate and return the key index
      } 
    } 
    digitalWrite(rowPins[r], HIGH); 
  } 
  return -1; // No key pressed
}

// Converts a string to a float and updates the end pointer
float my_strtof(const char *str, char **endptr) { 
  float result = 0.0; 
  bool negative = false; 
  const char *ptr = str;
  while (isspace(*ptr)) ptr++; // Skip initial whitespace
  if (*ptr == '-') { negative = true; ptr++; } // Handle negative sign
  else if (*ptr == '+') { ptr++; }
  while (isdigit(*ptr)) { 
    result = result * 10 + (*ptr - '0'); // Convert integer part
    ptr++;
  }
  if (*ptr == '.') { 
    ptr++; 
    float fraction = 0.0, divisor = 10.0; 
    while (isdigit(*ptr)) { 
      fraction += (*ptr - '0') / divisor; // Convert decimal part
      divisor *= 10.0; 
      ptr++;
    } 
    result += fraction;
  }
  if (endptr) *endptr = (char*)ptr; // Return pointer to next character
  return negative ? -result : result;
}

// Validates that the input contains only allowed characters
static inline bool validateInput(const char* input) { 
  const char* allowed = "0123456789 +-*/^.()Sxyz="; // Allowed character list
  for (const char *p = input; *p; p++) { 
    if (!isspace(*p) && !strchr(allowed, *p)) 
      return false; // Return false if an invalid character is found
  }
  return true;
}

// Global pointer for traversing the expression during parsing
const char *expr_ptr;

// Skips whitespace in the expression
void skipWhitespace() { 
  while (*expr_ptr == ' ') 
    expr_ptr++;
}

// Declaration of the function to parse the complete expression
float parseExpression();

// Parses a primary value (number, parenthesis, or square root)
float parsePrimary() { 
  skipWhitespace(); 
  if (*expr_ptr == '(') { 
    expr_ptr++; 
    float result = parseExpression(); // Evaluate expression within parentheses
    if (*expr_ptr == ')')
      expr_ptr++;
    return result;
  }
  if (strncmp(expr_ptr, SQRT_SYMBOL, strlen(SQRT_SYMBOL)) == 0) { 
    expr_ptr += strlen(SQRT_SYMBOL); 
    return sqrt(parsePrimary()); // Compute square root of the next primary value
  }
  char *end; 
  float result = my_strtof(expr_ptr, &end); // Convert string to float
  expr_ptr = end;
  return result;
}

// Parses exponentiation after a primary value
float parseFactor() { 
  float base = parsePrimary(); 
  skipWhitespace(); 
  if (*expr_ptr == '^') { 
    expr_ptr++; 
    float exponent = parseFactor(); // Calculate exponent
    return pow(base, exponent);
  } 
  return base;
}

// Parses multiplication and division in the expression
float parseTerm() { 
  float result = parseFactor(); 
  skipWhitespace(); 
  while (*expr_ptr == '*' || *expr_ptr == '/') { 
    char op = *expr_ptr++; 
    float factor = parseFactor(); 
    result = (op == '*') ? result * factor : result / factor; // Execute operation
    skipWhitespace();
  } 
  return result;
}

// Parses addition and subtraction in the expression
float parseExpression() { 
  float result = parseTerm(); 
  skipWhitespace(); 
  while (*expr_ptr == '+' || *expr_ptr == '-') { 
    char op = *expr_ptr++; 
    float term = parseTerm(); 
    result = (op == '+') ? result + term : result - term; // Perform addition or subtraction
    skipWhitespace();
  } 
  return result;
}

// Evaluates the arithmetic expression and returns the result
float evaluateExpression(const char *expr) { 
  if (!validateInput(expr))
    return NAN; // Return NaN if the input is invalid
  expr_ptr = expr; 
  return parseExpression();
}

// Evaluates the polynomial using Horner's method
float evaluatePoly(const float poly[], int degree, float x) {
  float result = poly[degree];
  for (int i = degree - 1; i >= 0; i--) {
    result = result * x + poly[i]; // Compute polynomial value for x
  }
  return result;
}

// Finds real roots of the polynomial using sampling and bisection
int findRealPolynomialRoots(const float poly[], int degree, float rootsFound[]) {
  float tol = TOLERANCE;
  float maxCoeff = 0;
  for (int i = 0; i < degree; i++) {
    float val = fabs(poly[i]);
    if (val > maxCoeff)
      maxCoeff = val; // Determine the largest coefficient for bounding
  }
  float bound = 1 + maxCoeff / fabs(poly[degree]); // Estimate the bound for x values
  int count = 0;
  const int samples = 200; // Number of sample points
  float step = (2 * bound) / samples; // Define the step size
  float prev_x = -bound;
  float prev_val = evaluatePoly(poly, degree, prev_x);
  for (int i = 1; i <= samples; i++) {
    float x = -bound + i * step;
    float val = evaluatePoly(poly, degree, x);
    if (fabs(val) < tol) {
      bool duplicate = false;
      for (int j = 0; j < count; j++) {
        if (fabs(rootsFound[j] - x) < tol) { 
          duplicate = true; 
          break; 
        }
      }
      if (!duplicate)
        rootsFound[count++] = x; // Store the unique root found
    }
    if (prev_val * val < 0) { // Indicates a sign change (possible root)
      float a = prev_x, b = x;
      float fa = prev_val, fb = val;
      for (int iter = 0; iter < 20; iter++) {
        float mid = (a + b) / 2;
        float fmid = evaluatePoly(poly, degree, mid);
        if (fabs(fmid) < tol) {
          a = mid;
          b = mid;
          break;
        }
        if (fa * fmid < 0) {
          b = mid;
          fb = fmid;
        } else {
          a = mid;
          fa = fmid;
        }
      }
      float root = (a + b) / 2;
      bool duplicate = false;
      for (int j = 0; j < count; j++) {
        if (fabs(rootsFound[j] - root) < tol) { 
          duplicate = true; 
          break;
        }
      }
      if (!duplicate)
        rootsFound[count++] = root; // Store the found root
    }
    prev_x = x;
    prev_val = val;
  }
  return count; // Return the number of roots found
}

// Processes one side of the polynomial equation to extract coefficients for each exponent
void processSidePoly(const char *side, float polyArr[], char var) {
  const char *p = side;
  while (*p) {
    while (*p == ' ')
      p++; // Skip spaces
    if (!*p) break;
    int sign = 1;
    if (*p == '+' || *p == '-') {
      if (*p == '-')
        sign = -1; // Adjust sign if '-' is found
      p++;
    }
    char *end;
    float coeff = my_strtof(p, &end); // Extract the coefficient
    if (end != p) {
      p = end;
    } else {
      coeff = 1; // Implicit coefficient of 1 if none is provided
    }
    coeff *= sign;
    int exponent = 0;
    if (*p == var || *p == toupper(var)) {
      p++; // Move past the variable character
      exponent = 1; // Default exponent is 1
      if (*p == '^') {
        p++;
        exponent = (int)my_strtof(p, &end); // Extract explicit exponent
        p = end;
      }
    }
    polyArr[exponent] += coeff; // Accumulate the coefficient for this exponent
  }
}

// Solves the polynomial equation in the form <LHS>=<RHS>
int solvePolynomialEquation(const char *eq, float roots[], char var) {
  if (!validateInput(eq))
    return -1; // Return error if input is invalid
  char eqCopy[33];
  strncpy(eqCopy, eq, 32);
  eqCopy[32] = '\0';
  char *equalSign = strchr(eqCopy, '=');
  if (!equalSign)
    return -1; // Return error if '=' is not found
  *equalSign = '\0';
  char *lhs = eqCopy, *rhs = equalSign + 1;
  
  float polyL[MAX_POLY_DEGREE+1] = {0};
  float polyR[MAX_POLY_DEGREE+1] = {0};
  
  processSidePoly(lhs, polyL, var); // Process left-hand side
  processSidePoly(rhs, polyR, var); // Process right-hand side
  
  if (isnan(polyL[0]) || isnan(polyR[0]))
    return -1; // Parsing error
  
  float poly[MAX_POLY_DEGREE+1];
  for (int i = 0; i <= MAX_POLY_DEGREE; i++) {
    poly[i] = polyL[i] - polyR[i]; // Bring all terms to one side
  }
  
  int degree = MAX_POLY_DEGREE;
  while (degree > 0 && fabs(poly[degree]) < TOLERANCE)
    degree--; // Determine the actual degree of the polynomial
  
  if (degree == 0) {
    if (fabs(poly[0]) < TOLERANCE)
      return -2; // Infinite solutions if constant term is zero
    else
      return 0;  // No solution if constant term is non-zero
  }
  if (degree == 1) {
    roots[0] = -poly[0] / poly[1]; // Solve linear equation
    return 1;
  }
  if (degree == 2) {
    float a = poly[2], b = poly[1], c = poly[0];
    float disc = b * b - 4 * a * c;
    if (disc < 0)
      return 0; // No real roots if discriminant is negative
    else if (fabs(disc) < TOLERANCE) {
      roots[0] = -b / (2 * a); // Double root
      return 1;
    } else {
      roots[0] = (-b + sqrt(disc)) / (2 * a);
      roots[1] = (-b - sqrt(disc)) / (2 * a);
      return 2; // Two distinct real roots
    }
  }
  // For degree ≥ 3, use sampling and bisection to find roots
  float foundRoots[MAX_POLY_DEGREE] = {0};
  int count = findRealPolynomialRoots(poly, degree, foundRoots);
  if (count == 0)
    return 0; // Return 0 if no roots were found
  else if (count == 1) {
    roots[0] = foundRoots[0];
    return 1;
  } else {
    roots[0] = foundRoots[0];
    roots[1] = foundRoots[1];
    return 2; // Return the first two roots found
  }
}

// Converts a float to a string, using scientific notation if needed
void formatResultFloat(float res, char *resultStr, int size) {
  char buffer[17];
  if ((fabs(res) >= 100000) || (fabs(res) > 0 && fabs(res) < 0.0001))
    snprintf(buffer, sizeof(buffer), "%.2e", res); // Use scientific notation for very large or small numbers
  else {
    dtostrf(res, 0, 2, buffer); // Convert with 2 decimal places
    char *dot = strchr(buffer, '.');
    if (dot != NULL && strcmp(dot, ".00") == 0)
      *dot = '\0'; // Remove ".00" if present
  }
  if (strlen(buffer) >= (unsigned)size) {
    snprintf(buffer, sizeof(buffer), "%.2e", res);
    if (strlen(buffer) >= (unsigned)size)
      buffer[size-1] = '\0'; // Ensure the string fits within the buffer
  }
  strncpy(resultStr, buffer, size);
  resultStr[size - 1] = '\0'; // Null-terminate the string
}
  
// Updates the input buffer with the computed result (non-editable mode)
void updateBufferWithResult(const char *result) {
  strncpy(eqInputBuffer, result, 17);
  eqInputBuffer[16] = '\0';
  eqInputLength = strlen(eqInputBuffer);
  cursorPosition = eqInputLength;
}

// Displays a message on the interface and disables editing until dismissed
void displayMessage(const char *msg) {
  updateBufferWithResult(msg);
  messageDisplayed = true;
}

// Multitap keypad settings for entering special characters
#define MULTITAP_TIMEOUT 2500 // Timeout (ms) for multitap confirmation
#define BLINK_INTERVAL 500    // Interval (ms) for cursor blinking
int currentKey_input = -1;
int currentTokenIndex_input = 0;
unsigned long lastKeyPressTime_input = 0;
bool multiTapActive_input = false;
unsigned long lastBlinkTime_input = 0;
bool blinkState_input = false;

// Key mapping for multitap options
const char* key0Options_input[] = {"1"};
const char* key1Options_input[] = {"2"};
const char* key2Options_input[] = {"3"};
const char* key3Options_input[] = {"4"};
const char* key4Options_input[] = {"5"};
const char* key5Options_input[] = {"6"};
const char* key6Options_input[] = {"7"};
const char* key7Options_input[] = {"8"};
const char* key8Options_input[] = {"9"};
const char* key9Options_input[] = {"0"};
const char* key10Options_input[] = {};
const char* key11Options_input[] = {};
const char* key12Options_input[] = {"x", "y", "z"};
const char* key13Options_input[] = {"+", "-", "*", "/", "(", ")", ".", "=", "S", "^"};
const char* key14Options_input[] = {};
const char* key15Options_input[] = {};
struct KeyMapping_input { const char **options; uint8_t numOptions; };
KeyMapping_input eqKeyMap_input[16] = {
  { key0Options_input, 1 }, { key1Options_input, 1 }, { key2Options_input, 1 }, { key3Options_input, 1 },
  { key4Options_input, 1 }, { key5Options_input, 1 }, { key6Options_input, 1 }, { key7Options_input, 1 },
  { key8Options_input, 1 }, { key9Options_input, 1 }, { key10Options_input, 1 }, { key11Options_input, 1 },
  { key12Options_input, 3 },
  { key13Options_input, 10 }, { key14Options_input, 1 }, { key15Options_input, 1 }
};

// Checks if the current key is a multitap key (keys 12 and 13)
static inline bool isMultiTapKey(int keyIndex) { 
  return (keyIndex == 12 || keyIndex == 13); 
}

// Inserts the selected token into the input buffer at the cursor position
void insertTokenAtCursor_input(const char *token) {
  uint8_t tokenLen = strlen(token);
  if (eqInputLength + tokenLen <= 16) {
    for (int i = eqInputLength; i >= cursorPosition; i--) {
      eqInputBuffer[i + tokenLen] = eqInputBuffer[i]; // Shift characters to make space
    }
    for (int i = 0; i < tokenLen; i++) {
      eqInputBuffer[cursorPosition + i] = token[i]; // Insert the token into the buffer
    }
    eqInputLength += tokenLen;
    cursorPosition += tokenLen; // Update cursor position
  }
}

// Removes the token immediately before the cursor position
void removeTokenAtCursor_input() {
  if (cursorPosition > 0) {
    for (int i = cursorPosition - 1; i < eqInputLength; i++) {
      eqInputBuffer[i] = eqInputBuffer[i + 1]; // Shift characters left
    }
    eqInputLength--;
    cursorPosition--;
  }
}

// Moves the cursor one position to the left
void moveCursorLeft_input() {
  if (cursorPosition > 0)
    cursorPosition--;
}

// Moves the cursor one position to the right
void moveCursorRight_input() {
  if (cursorPosition < eqInputLength)
    cursorPosition++;
}

// Updates the equation display line on the LCD, with a blinking cursor
void updateEquationDisplay_input() {
  if (millis() - lastBlinkTime_input >= BLINK_INTERVAL) {
    blinkState_input = !blinkState_input; // Toggle the cursor state (visible/invisible)
    lastBlinkTime_input = millis();
  }
  char displayBuffer[17];
  memset(displayBuffer, ' ', 16); // Fill the buffer with spaces
  displayBuffer[16] = '\0';
  memcpy(displayBuffer, eqInputBuffer, eqInputLength); // Copy the equation into the display buffer
  if (multiTapActive_input) {
    const char *token = eqKeyMap_input[currentKey_input].options[currentTokenIndex_input];
    uint8_t tokenLen = strlen(token);
    if (cursorPosition + tokenLen <= 16)
      memcpy(displayBuffer + cursorPosition, token, tokenLen); // Show current multitap option
  }
  else {
    char original = (cursorPosition < eqInputLength) ? eqInputBuffer[cursorPosition] : ' ';
    displayBuffer[cursorPosition] = blinkState_input ? '_' : original; // Display blinking cursor
  }
  lcdSetCursor(0, 1);
  lcdPrint(displayBuffer); // Update the LCD with the current display buffer
}

// Processes the pressed key and executes the corresponding action
void processKeyPress_input(int keyIndex) {
  if (messageDisplayed) { // If a non-editable message is displayed
    if (keyIndex == 11) { // Only the ENTER key dismisses the message
      eqInputBuffer[0] = '\0';
      eqInputLength = 0;
      cursorPosition = 0;
      messageDisplayed = false;
    }
    return;
  }
  unsigned long now = millis();
  if (keyIndex == 10 || keyIndex == 11 || keyIndex == 14 || keyIndex == 15) {
    if (multiTapActive_input) {
      insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]); // Finalize multitap entry
      multiTapActive_input = false;
    }
    if (keyIndex == 10)
      removeTokenAtCursor_input(); // Remove the previous character
    else if (keyIndex == 11) {
      char resultStr[17];
      // If the input contains a variable, treat it as a polynomial equation
      if (strchr(eqInputBuffer, 'x') || strchr(eqInputBuffer, 'y') || strchr(eqInputBuffer, 'z')) {
        char var;
        if (strchr(eqInputBuffer, 'x'))
          var = 'x';
        else if (strchr(eqInputBuffer, 'y'))
          var = 'y';
        else if (strchr(eqInputBuffer, 'z'))
          var = 'z';
        else {
          displayMessage(ERROR_MSG);
          return;
        }
        float roots[2];
        int numRoots = solvePolynomialEquation(eqInputBuffer, roots, var);
        if (numRoots == -1)
          displayMessage(ERROR_MSG); // Parsing error in equation
        else if (numRoots == -2)
          displayMessage(INF_SOL_MSG); // Equation has infinite solutions
        else if (numRoots == 0)
          displayMessage(NOSOL_MSG); // No real solution found
        else if (numRoots == 1) {
          formatResultFloat(roots[0], resultStr, 17); // Format the single root found
          updateBufferWithResult(resultStr);
        }
        else if (numRoots >= 2) {
          char temp1[9], temp2[9];
          formatResultFloat(roots[0], temp1, 9);
          formatResultFloat(roots[1], temp2, 9);
          snprintf(resultStr, 17, "%c=%s %c=%s", var, temp1, var, temp2); // Prepare string with two roots
          displayMessage(resultStr);
          // Envia o resultado via Serial se estiver conectado a um computador
          sendRootsToSerial(roots[0], roots[1]);
        }
      }
      else {
        float res = evaluateExpression(eqInputBuffer); // Evaluate arithmetic expression
        if (isnan(res))
          displayMessage(ERROR_MSG); // Display error if expression is invalid
        else if (isinf(res))
          displayMessage(INF_SOL_MSG); // Display message for infinite result
        else {
          formatResultFloat(res, resultStr, 17); // Format the expression result
          updateBufferWithResult(resultStr);
        }
      }
    }
    else if (keyIndex == 14)
      moveCursorLeft_input(); // Move cursor left
    else if (keyIndex == 15)
      moveCursorRight_input(); // Move cursor right
    return;
  }
  if (!isMultiTapKey(keyIndex)) {
    insertTokenAtCursor_input(eqKeyMap_input[keyIndex].options[0]); // Insert character (non-multitap)
    lastKeyPressTime_input = now;
    return;
  }
  if (multiTapActive_input && keyIndex == currentKey_input && (now - lastKeyPressTime_input < MULTITAP_TIMEOUT))
    currentTokenIndex_input = (currentTokenIndex_input + 1) % eqKeyMap_input[keyIndex].numOptions; // Cycle multitap options
  else {
    if (multiTapActive_input)
      insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]); // Confirm previous multitap option
    currentKey_input = keyIndex;
    currentTokenIndex_input = 0;
    multiTapActive_input = isMultiTapKey(keyIndex); // Activate multitap for designated keys
  }
  lastKeyPressTime_input = now;
}

// Checks if the multitap timeout has expired and confirms the selected option
void checkMultiTapTimeout_input() {
  if (multiTapActive_input && isMultiTapKey(currentKey_input) && (millis() - lastKeyPressTime_input >= MULTITAP_TIMEOUT)) {
    insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]);
    multiTapActive_input = false;
  }
}

// Função para enviar as raízes via Serial, caso o Arduino esteja conectado ao PC
void sendRootsToSerial(float root1, float root2) {
  // Verifica se a porta serial está disponível para transmissão
  if (UCSR0A & (1 << UDRE0)) {
    char buffer1[10], buffer2[10];
    formatResultFloat(root1, buffer1, sizeof(buffer1));
    formatResultFloat(root2, buffer2, sizeof(buffer2));

    // Envia as raízes para o computador via Serial
    for (char *p = buffer1; *p; p++) {
      while (!(UCSR0A & (1 << UDRE0)));
      UDR0 = *p;
    }
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = ' '; // Espaço separador
    for (char *p = buffer2; *p; p++) {
      while (!(UCSR0A & (1 << UDRE0)));
      UDR0 = *p;
    }
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = '\n'; // Nova linha para indicar fim da transmissão
  }
}

// Setup function: initializes LCD, keypad, and control variables
void setup() {
  lcdInit();
  keypadInit();
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("SciCalc-Equation"); // Initial message on the LCD

  // Configura a comunicação Serial (caso o Arduino esteja conectado ao PC)
  UBRR0H = 0; // Baud rate 9600
  UBRR0L = 103;
  UCSR0B = (1 << TXEN0); // Habilita transmissão
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Formato de 8 bits, sem paridade

  eqInputBuffer[0] = '\0';
  eqInputLength = 0;
  cursorPosition = 0;
  lastBlinkTime_input = millis();
  blinkState_input = false;
}

// Main loop: processes keypad input and updates the display
void loop() {
  int keyIndex = getKeyIndex();
  if (keyIndex != -1)
    processKeyPress_input(keyIndex);
  checkMultiTapTimeout_input();
  updateEquationDisplay_input();
}

#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SQRT_SYMBOL "S"

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

static inline void lcdSendNibble(uint8_t nibble) {
  PORTC = (PORTC & ~((1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5)))
          | ((nibble & 0x0F) << 2);
  PORTD |= LCD_E_MASK;
  delayMicroseconds(1);
  PORTD &= ~LCD_E_MASK;
  delayMicroseconds(80);
}

static inline void lcdCommand(uint8_t cmd) {
  PORTC &= ~((1 << PC1)|(1 << PC0));
  lcdSendNibble(cmd >> 4);
  lcdSendNibble(cmd & 0x0F);
  delay(2);
}

static inline void lcdWriteData(uint8_t data) {
  PORTC = (PORTC & ~((1 << PC1)|(1 << PC0))) | (1 << PC1);
  lcdSendNibble(data >> 4);
  lcdSendNibble(data & 0x0F);
  delay(2);
}

void lcdInit() {
  DDRC |= (1 << PC0)|(1 << PC1)|(1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5);
  DDRD |= (1 << PD2);
  PORTC &= ~((1 << PC0)|(1 << PC1)|(1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5));
  PORTD &= ~LCD_E_MASK;
  delay(50);
  lcdCommand(0x01); 
  delay(2);
  lcdSendNibble(0x03);
  delay(5);
  lcdSendNibble(0x03);
  delay(5);
  lcdSendNibble(0x03);
  delayMicroseconds(150);
  lcdSendNibble(0x02);
  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdCommand(0x01);
  delay(2);
  lcdCommand(0x06);
  lcdCommand(0x0C);
}

void lcdClear() {
  lcdCommand(0x01);
  delay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + row_offsets[row]));
}

void lcdPrint(const char *str) {
  while (*str)
    lcdWriteData(*str++);
}

const byte rowPins[4] = {7, 8, 9, 10};
const byte colPins[4] = {3, 4, 5, 6};

void keypadInit() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }
}

int getKeyIndex() {
  for (int r = 0; r < 4; r++) {
    digitalWrite(rowPins[r], LOW);
    uint8_t portD = PIND;
    for (int c = 0; c < 4; c++) {
      if (!(portD & (1 << colPins[c]))) {
        delay(50);
        while (!(PIND & (1 << colPins[c])));
        digitalWrite(rowPins[r], HIGH);
        return r * 4 + c; 
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  return -1;
}

float my_strtof(const char *str, char **endptr) {
  float result = 0.0;
  bool negative = false;
  const char *ptr = str;
  while (isspace(*ptr)) ptr++;
  if(*ptr == '-') { negative = true; ptr++; }
  else if(*ptr == '+') { ptr++; }
  while(isdigit(*ptr)) {
    result = result * 10 + (*ptr - '0');
    ptr++;
  }
  if(*ptr == '.') {
    ptr++;
    float fraction = 0.0, divisor = 10.0;
    while(isdigit(*ptr)) {
      fraction += (*ptr - '0') / divisor;
      divisor *= 10.0;
      ptr++;
    }
    result += fraction;
  }
  if(endptr)
    *endptr = (char*)ptr;
  return negative ? -result : result;
}

const char *expr_ptr;
void skipWhitespace() {
  while (*expr_ptr == ' ') expr_ptr++;
}

// Analisa a entrada primária (operações básicas)
float parsePrimary() {
  skipWhitespace();
  if (*expr_ptr == '(') {
    expr_ptr++;
    float result = parseExpression();
    if (*expr_ptr == ')') expr_ptr++;
    return result;
  }
  if (strncmp(expr_ptr, SQRT_SYMBOL, strlen(SQRT_SYMBOL)) == 0) {
    expr_ptr += strlen(SQRT_SYMBOL); 
    float value = parsePrimary(); 
    return sqrt(value);
  }
  char *end;
  float result = my_strtof(expr_ptr, &end);
  expr_ptr = end;
  return result;
}

// Analisa fatores matemáticos (potências e raízes)
float parseFactor() {
  float base = parsePrimary();
  skipWhitespace();
  if (*expr_ptr == '^') {
    expr_ptr++;
    float exponent = parseFactor();
    return pow(base, exponent);
  }
  return base;
}

// Analisa termos (multiplicação e divisão)
float parseTerm() {
  float result = parseFactor();
  skipWhitespace();
  while (*expr_ptr == '*' || *expr_ptr == '/') {
    char op = *expr_ptr;
    expr_ptr++;
    float factor = parseFactor();
    result = (op == '*') ? result * factor : result / factor;
    skipWhitespace();
  }
  return result;
}

// Analisa expressões (soma e subtração)
float parseExpression() {
  float result = parseTerm();
  skipWhitespace();
  while (*expr_ptr == '+' || *expr_ptr == '-') {
    char op = *expr_ptr;
    expr_ptr++;
    float term = parseTerm();
    result = (op == '+') ? result + term : result - term;
    skipWhitespace();
  }
  return result;
}

float evaluateExpression(const char *expr) {
  expr_ptr = expr;
  return parseExpression();
}

void parseSide(const char *side, float *ax, float *by, float *cz, float *cons) {
  *ax = *by = *cz = *cons = 0;
  int sign = 1;
  const char *p = side;
  while (*p) {
    if (*p == ' ') { p++; continue; }
    if (*p == '+') { sign = 1; p++; continue; }
    if (*p == '-') { sign = -1; p++; continue; }
    char *end;
    float num = my_strtof(p, &end);
    bool numberPresent = (end != p);
    if (numberPresent) { p = end; } else { num = 1; }
    if (*p == 'x' || *p == 'X') { *ax += sign * num; p++; }
    else if (*p == 'y' || *p == 'Y') { *by += sign * num; p++; }
    else if (*p == 'z' || *p == 'Z') { *cz += sign * num; p++; }
    else { *cons += sign * num; }
  }
}

void parseEquationLinear(const char *eq, float coeff[4]) {
  char buffer[33];
  strncpy(buffer, eq, 32);
  buffer[32] = '\0';
  char *equalSign = strchr(buffer, '=');
  if (equalSign == NULL) { coeff[0]=coeff[1]=coeff[2]=coeff[3]=0; return; }
  *equalSign = '\0';
  char *lhs = buffer;
  char *rhs = equalSign + 1;
  float ax_lhs, by_lhs, cz_lhs, cons_lhs;
  float ax_rhs, by_rhs, cz_rhs, cons_rhs;
  parseSide(lhs, &ax_lhs, &by_lhs, &cz_lhs, &cons_lhs);
  parseSide(rhs, &ax_rhs, &by_rhs, &cz_rhs, &cons_rhs);
  coeff[0] = ax_lhs - ax_rhs;
  coeff[1] = by_lhs - by_rhs;
  coeff[2] = cz_lhs - cz_rhs;
  coeff[3] = cons_rhs - cons_lhs;
}

float solveLinearEquation(const char *eq, bool *valid) {
  char eqCopy[33];
  strncpy(eqCopy, eq, 32);
  eqCopy[32] = '\0';
  char *equalSign = strchr(eqCopy, '=');
  if (equalSign == NULL) { *valid = false; return 0.0; }
  *equalSign = '\0';
  char *lhs = eqCopy;
  char *rhs = equalSign + 1;
  float ax_lhs, dummy1, dummy2, cons_lhs;
  float ax_rhs, dummy3, dummy4, cons_rhs;
  parseSide(lhs, &ax_lhs, &dummy1, &dummy2, &cons_lhs);
  parseSide(rhs, &ax_rhs, &dummy3, &dummy4, &cons_rhs);
  float A = ax_lhs - ax_rhs;
  float B = cons_lhs - cons_rhs;
  if (fabs(A) < 1e-6) { *valid = false; return 0.0; }
  *valid = true;
  return -B / A;
}

bool solveLinearSystem(float mat[3][4], int n, float sol[3]) {
  for (int i = 0; i < n; i++) {
    int pivot = i;
    for (int j = i+1; j < n; j++) {
      if (fabs(mat[j][i]) > fabs(mat[pivot][i]))
        pivot = j;
    }
    if (fabs(mat[pivot][i]) < 1e-6)
      return false;
    if (pivot != i) {
      for (int k = i; k <= n; k++) {
        float temp = mat[i][k];
        mat[i][k] = mat[pivot][k];
        mat[pivot][k] = temp;
      }
    }
    float div = mat[i][i];
    for (int k = i; k <= n; k++) {
      mat[i][k] /= div;
    }
    for (int j = i+1; j < n; j++) {
      float factor = mat[j][i];
      for (int k = i; k <= n; k++) {
        mat[j][k] -= factor * mat[i][k];
      }
    }
  }
  for (int i = n-1; i >= 0; i--) {
    sol[i] = mat[i][n];
    for (int j = i+1; j < n; j++) {
      sol[i] -= mat[i][j] * sol[j];
    }
  }
  return true;
}

bool solveEquationSystem(const char *eqStr, float sol[3], int *numEq) {
  char buffer[65];
  strncpy(buffer, eqStr, 64);
  buffer[64] = '\0';
  char *eqs[3];
  int count = 0;
  char *token = strtok(buffer, ";");
  while(token != NULL && count < 3) {
    eqs[count++] = token;
    token = strtok(NULL, ";");
  }
  *numEq = count;
  if (count < 1) return false;
  float mat[3][4] = {0};
  for (int i = 0; i < count; i++) {
    float coeff[4];
    parseEquationLinear(eqs[i], coeff);
    mat[i][0] = coeff[0];
    mat[i][1] = coeff[1];
    mat[i][2] = coeff[2];
    mat[i][3] = coeff[3];
  }
  return solveLinearSystem(mat, count, sol);
}

void formatResultFloat(float res, char *resultStr, int size) {
  char buffer[17];
  dtostrf(res, 0, 2, buffer);
  char *dot = strchr(buffer, '.');
  if(dot != NULL && strcmp(dot, ".00") == 0) {
    *dot = '\0';
  }
  strncpy(resultStr, buffer, size);
  resultStr[size-1] = '\0';
}

#define MULTITAP_TIMEOUT 2500
#define BLINK_INTERVAL 500  

char eqInputBuffer[17] = "";
uint8_t eqInputLength = 0;
uint8_t cursorPosition = 0;

int currentKey_input = -1;
int currentTokenIndex_input = 0;
unsigned long lastKeyPressTime_input = 0;
bool multiTapActive_input = false;

unsigned long lastBlinkTime_input = 0;
bool blinkState_input = false;

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
const char* key10Options_input[] = {"BK"};
const char* key11Options_input[] = {"ENT"};
const char* key12Options_input[] = {"x", "y", "z"};
const char* key13Options_input[] = {"+", "-", "*", "/", "(", ")", ".", "=", "S", "^"};
const char* key14Options_input[] = {"LFT"};
const char* key15Options_input[] = {"RGT"};

struct KeyMapping_input {
  const char **options;
  uint8_t numOptions;
};

KeyMapping_input eqKeyMap_input[16] = {
  { key0Options_input, 1 },
  { key1Options_input, 1 },
  { key2Options_input, 1 },
  { key3Options_input, 1 },
  { key4Options_input, 1 },
  { key5Options_input, 1 },
  { key6Options_input, 1 },
  { key7Options_input, 1 },
  { key8Options_input, 1 },
  { key9Options_input, 1 },
  { key10Options_input, 1 },  
  { key11Options_input, 1 },  
  { key12Options_input, 3 },  
  { key13Options_input, 10 }, 
  { key14Options_input, 1 },  
  { key15Options_input, 1 }   
};

bool isMultiTapKey(int keyIndex) {
  return (keyIndex == 12 || keyIndex == 13);
}

void insertTokenAtCursor_input(const char *token) {
  uint8_t tokenLen = strlen(token);
  if (eqInputLength + tokenLen <= 16) {
    for (int i = eqInputLength; i >= cursorPosition; i--) {
      eqInputBuffer[i + tokenLen] = eqInputBuffer[i];
    }
    for (int i = 0; i < tokenLen; i++) {
      eqInputBuffer[cursorPosition + i] = token[i];
    }
    eqInputLength += tokenLen;
    cursorPosition += tokenLen;
  }
}

void removeTokenAtCursor_input() {
  if (cursorPosition > 0) {
    for (int i = cursorPosition - 1; i < eqInputLength; i++) {
      eqInputBuffer[i] = eqInputBuffer[i + 1];
    }
    eqInputLength--;
    cursorPosition--;
  }
}

void moveCursorLeft_input() {
  if (cursorPosition > 0) cursorPosition--;
}

void moveCursorRight_input() {
  if (cursorPosition < eqInputLength) cursorPosition++;
}

void updateEquationDisplay_input() {
  if (millis() - lastBlinkTime_input >= BLINK_INTERVAL) {
    blinkState_input = !blinkState_input;
    lastBlinkTime_input = millis();
  }
  
  char displayBuffer[17];
  memset(displayBuffer, ' ', 16);
  displayBuffer[16] = '\0';
  memcpy(displayBuffer, eqInputBuffer, eqInputLength);
  
  if (multiTapActive_input) {
    const char *token = eqKeyMap_input[currentKey_input].options[currentTokenIndex_input];
    uint8_t tokenLen = strlen(token);
    if (cursorPosition + tokenLen <= 16) {
      memcpy(displayBuffer + cursorPosition, token, tokenLen);
    }
  } else {
    char original = (cursorPosition < eqInputLength) ? eqInputBuffer[cursorPosition] : ' ';
    displayBuffer[cursorPosition] = blinkState_input ? '_' : original;
  }
  
  lcdSetCursor(0, 1);
  lcdPrint(displayBuffer);
}

void processKeyPress_input(int keyIndex) {
  unsigned long now = millis();
  if (keyIndex == 10 || keyIndex == 11 || keyIndex == 14 || keyIndex == 15) {
    if (multiTapActive_input) {
      insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]);
      multiTapActive_input = false;
    }
    if (keyIndex == 10) { 
      removeTokenAtCursor_input();
    } else if (keyIndex == 11) { 
      if (strchr(eqInputBuffer, ';') != NULL) {
        float sol[3] = {0,0,0};
        int numEq = 0;
        bool ok = solveEquationSystem(eqInputBuffer, sol, &numEq);
        char resultStr[17];
        if (ok) {
          if (numEq == 1) {
            formatResultFloat(sol[0], resultStr, 17);
          } else {
            char temp1[9], temp2[9];
            formatResultFloat(sol[0], temp1, 9);
            formatResultFloat(sol[1], temp2, 9);
            snprintf(resultStr, 17, "x=%s y=%s", temp1, temp2);
          }
          strcpy(eqInputBuffer, resultStr);
          eqInputLength = strlen(resultStr);
          cursorPosition = eqInputLength;
        } else {
          strcpy(eqInputBuffer, "Err");
          eqInputLength = 3;
          cursorPosition = eqInputLength;
        }
      } else if (strchr(eqInputBuffer, 'x') || strchr(eqInputBuffer, 'X') ||
                 strchr(eqInputBuffer, 'y') || strchr(eqInputBuffer, 'Y') ||
                 strchr(eqInputBuffer, 'z') || strchr(eqInputBuffer, 'Z')) {
        bool valid;
        float res = solveLinearEquation(eqInputBuffer, &valid);
        char resultStr[17];
        if (valid) {
          formatResultFloat(res, resultStr, 17);
          strcpy(eqInputBuffer, resultStr);
          eqInputLength = strlen(resultStr);
          cursorPosition = eqInputLength;
        } else {
          strcpy(eqInputBuffer, "Err");
          eqInputLength = 3;
          cursorPosition = eqInputLength;
        }
      } else {
        float res = evaluateExpression(eqInputBuffer);
        char resultStr[17];
        formatResultFloat(res, resultStr, 17);
        strcpy(eqInputBuffer, resultStr);
        eqInputLength = strlen(resultStr);
        cursorPosition = eqInputLength;
      }
    } else if (keyIndex == 14) { 
      moveCursorLeft_input();
    } else if (keyIndex == 15) { 
      moveCursorRight_input();
    }
    return;
  }
  
  if (!isMultiTapKey(keyIndex)) {
    insertTokenAtCursor_input(eqKeyMap_input[keyIndex].options[0]);
    lastKeyPressTime_input = now;
    return;
  }
  
  if (multiTapActive_input && keyIndex == currentKey_input && (now - lastKeyPressTime_input < MULTITAP_TIMEOUT)) {
    currentTokenIndex_input = (currentTokenIndex_input + 1) % eqKeyMap_input[keyIndex].numOptions;
  } else {
    if (multiTapActive_input) {
      insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]);
    }
    currentKey_input = keyIndex;
    currentTokenIndex_input = 0;
    multiTapActive_input = isMultiTapKey(keyIndex);
  }
  lastKeyPressTime_input = now;
}

void checkMultiTapTimeout_input() {
  if (multiTapActive_input && isMultiTapKey(currentKey_input) && (millis() - lastKeyPressTime_input >= MULTITAP_TIMEOUT)) {
    insertTokenAtCursor_input(eqKeyMap_input[currentKey_input].options[currentTokenIndex_input]);
    multiTapActive_input = false;
  }
}

// Configuração inicial
void setup() {
  lcdInit();
  keypadInit();
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("SciCalc-Equation");
  
  eqInputBuffer[0] = '\0';
  eqInputLength = 0;
  cursorPosition = 0;
  lastBlinkTime_input = millis();
  blinkState_input = false;
}

// Loop principal
void loop() {
  int keyIndex = getKeyIndex();
  if (keyIndex != -1) {
    processKeyPress_input(keyIndex);
  }
  checkMultiTapTimeout_input();
  updateEquationDisplay_input();
}

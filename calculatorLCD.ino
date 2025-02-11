#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// LCD pin definitions
#define LCD_RS_PIN  A1
#define LCD_RW_PIN  A0
#define LCD_E_PIN   2
#define LCD_D4_PIN  A2
#define LCD_D5_PIN  A3
#define LCD_D6_PIN  A4
#define LCD_D7_PIN  A5

// LCD mask definitions
#define LCD_RS_MASK (1<<PC1)
#define LCD_RW_MASK (1<<PC0)
#define LCD_D4_MASK (1<<PC2)
#define LCD_D5_MASK (1<<PC3)
#define LCD_D6_MASK (1<<PC4)
#define LCD_D7_MASK (1<<PC5)
#define LCD_E_MASK  (1<<PD2)

// Operation string stored in flash (nova operação "lin" adicionada ao final)
const char opStr[] PROGMEM = "+,-,*,/,sqrt,sin,cos,tan,log,pow,fact,exp,log10,log2,asin,acos,atan,sinh,cosh,tanh,cbrt,%,lin";

// --- LCD Functions ---

// Send 4-bit nibble to LCD
static inline void lcdSendNibble(uint8_t nibble) {
  PORTC = (PORTC & ~((1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5)))
          | ((nibble & 0x0F) << 2);
  PORTD |= LCD_E_MASK;
  delayMicroseconds(1);
  PORTD &= ~LCD_E_MASK;
  delayMicroseconds(80);
}

// Send command to LCD
static inline void lcdCommand(uint8_t cmd) {
  PORTC &= ~((1 << PC1) | (1 << PC0));
  lcdSendNibble(cmd >> 4);
  lcdSendNibble(cmd & 0x0F);
  delay(2);
}

// Write data to LCD
static inline void lcdWriteData(uint8_t data) {
  PORTC = (PORTC & ~((1 << PC1) | (1 << PC0))) | (1 << PC1);
  lcdSendNibble(data >> 4);
  lcdSendNibble(data & 0x0F);
  delay(2);
}

// Initialize the LCD
void lcdInit() {
  DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);
  DDRD |= (1 << PD2);
  PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5));
  PORTD &= ~LCD_E_MASK;
  delay(50);
  PORTC &= ~(1 << PC1);
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

// Clear the LCD screen
void lcdClear() {
  lcdCommand(0x01);
  delay(2);
}

// Set the LCD cursor position
void lcdSetCursor(uint8_t col, uint8_t row) {
  const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + row_offsets[row]));
}

// Print a string from RAM
void lcdPrint(const char *str) {
  while (*str)
    lcdWriteData(*str++);
}

// Print a string from flash
void lcdPrintF(const __FlashStringHelper *str) {
  PGM_P p = (PGM_P)str;
  char c;
  while ((c = pgm_read_byte(p++)))
    lcdWriteData(c);
}

// Print the selected operation directly from flash
void lcdPrintOperation(uint8_t index) {
  uint8_t currentIndex = 0;
  uint16_t i = 0;
  char c;
  while (true) {
    c = pgm_read_byte_near(opStr + i);
    if (c == '\0') break;         // End of string
    if (currentIndex == index) {
      // Print until a comma or fim de string seja alcançado
      while (c != ',' && c != '\0') {
        lcdWriteData(c);
        i++;
        c = pgm_read_byte_near(opStr + i);
      }
      break;
    } else {
      // Pula a operação atual
      while (c != ',' && c != '\0') {
        i++;
        c = pgm_read_byte_near(opStr + i);
      }
      if (c == ',') {
        currentIndex++;
        i++;  // Pula a vírgula
      } else {
        break;
      }
    }
  }
}

// --- Keypad Functions ---

const byte rowPins[4] = {7, 8, 9, 10};
const byte colPins[4] = {3, 4, 5, 6};
const char keymap[4][4] = {
  {'1', '2', '3', '4'},
  {'5', '6', '7', '8'},
  {'9', '0', 'E', '='},
  {'-', '.', 'L', 'R'}
};

// Initialize keypad pins
void keypadInit() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }
}

// Read a key from the keypad
char getKey() {
  for (int r = 0; r < 4; r++) {
    digitalWrite(rowPins[r], LOW);
    uint8_t portD = PIND;
    for (int c = 0; c < 4; c++) {
      if (!(portD & (1 << colPins[c]))) {
        delay(50);
        while (!(PIND & (1 << colPins[c])));
        digitalWrite(rowPins[r], HIGH);
        return keymap[r][c];
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  return 0;
}

// --- Calculator Variables and Functions ---

enum CalcState {
  WAIT_OPERAND1,
  SELECT_OPERATION,
  WAIT_OPERAND2,
  SHOW_RESULT
};
CalcState calcState = WAIT_OPERAND1;

char inputBuffer[17] = "";
uint8_t inputLength = 0;
bool hasDecimalPoint = false;
bool isNegative = false;
float operand1 = 0.0, operand2 = 0.0, result = 0.0;
uint8_t menuIndex = 0;
// Atualizado para 23 operações (nova operação incluída)
const uint8_t NUM_OPERATIONS = 23;

// Bitmask para operações binárias (nova operação "lin" incluída, índice 22)
const uint32_t binaryOpsMask = (1UL << 0) | (1UL << 1) | (1UL << 2) |
                               (1UL << 3) | (1UL << 9) | (1UL << 21) | (1UL << 22);

// Adiciona um caractere no buffer de entrada
void addCharToInput(char c) {
  if (inputLength < 16) {
    inputBuffer[inputLength++] = c;
    inputBuffer[inputLength] = '\0';
  }
}

// Remove o último caractere do buffer de entrada
void removeCharFromInput() {
  if (inputLength > 0) {
    inputLength--;
    inputBuffer[inputLength] = '\0';
  }
}

// Calcula o fatorial (como float)
float factorial(float n) {
  if (n <= 1.0)
    return 1.0;
  float f = 1.0;
  for (int i = 2; i <= (int)n; i++)
    f *= i;
  return f;
}

// Realiza o cálculo baseado na operação selecionada
float calculate() {
  float res = 0.0;
  switch (menuIndex) {
    case 0:
      res = operand1 + operand2;
      break;
    case 1:
      res = operand1 - operand2;
      break;
    case 2:
      res = operand1 * operand2;
      break;
    case 3:
      if (operand2 != 0.0)
        res = operand1 / operand2;
      else {
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintF(F("Error: Div 0"));
        delay(2000);
        resetCalculator();
        return 0.0;
      }
      break;
    case 4:
      res = sqrt(operand1);
      break;
    case 5:
      res = sin(operand1);
      break;
    case 6:
      res = cos(operand1);
      break;
    case 7:
      res = tan(operand1);
      break;
    case 8:
      res = log(operand1);
      break;
    case 9:
      res = pow(operand1, operand2);
      break;
    case 10:
      res = factorial(operand1);
      break;
    case 11:
      res = exp(operand1);
      break;
    case 12:
      res = log10(operand1);
      break;
    case 13:
      res = log(operand1) / log(2);
      break;
    case 14:
      res = asin(operand1);
      break;
    case 15:
      res = acos(operand1);
      break;
    case 16:
      res = atan(operand1);
      break;
    case 17:
      res = sinh(operand1);
      break;
    case 18:
      res = cosh(operand1);
      break;
    case 19:
      res = tanh(operand1);
      break;
    case 20:
      res = cbrt(operand1);
      break;
    case 21:
      if (operand2 != 0.0)
        res = fmod(operand1, operand2);
      else {
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintF(F("Error: Div 0"));
        delay(2000);
        resetCalculator();
        return 0.0;
      }
      break;
    case 22:  // Nova operação: Equação Linear (ax + b = 0)
      if (operand1 != 0.0)
        res = -operand2 / operand1;  // x = -b/a
      else {
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintF(F("Error: a=0"));
        delay(2000);
        resetCalculator();
        return 0.0;
      }
      break;
    default:
      break;
  }
  return res;
}

// Processa as teclas para entrada de operandos
void processOperandKey(char key) {
  if (key >= '0' && key <= '9')
    addCharToInput(key);
  else if (key == '.') {
    if (!hasDecimalPoint) {
      addCharToInput('.');
      hasDecimalPoint = true;
    }
  }
  else if (key == '-') {
    isNegative = !isNegative;
  }
  else if (key == 'E') {
    if (inputLength > 0) {
      if (inputBuffer[inputLength - 1] == '.')
        hasDecimalPoint = false;
      removeCharFromInput();
    }
    else {
      if (calcState == WAIT_OPERAND2)
        calcState = SELECT_OPERATION;
      else if (calcState == WAIT_OPERAND1) {
        resetCalculator();
        return;
      }
    }
  }
  else if (key == '=') {
    float value = atof(inputBuffer);
    if (isNegative)
      value = -value;
    if (calcState == WAIT_OPERAND1) {
      operand1 = value;
      calcState = SELECT_OPERATION;
    }
    else if (calcState == WAIT_OPERAND2) {
      operand2 = value;
      result = calculate();
      calcState = SHOW_RESULT;
    }
    inputBuffer[0] = '\0';
    inputLength = 0;
    hasDecimalPoint = false;
    isNegative = false;
  }
  updateDisplay();
}

// Processa as teclas para selecionar operações
void processOperationKey(char key) {
  if (key == 'L')
    menuIndex = (menuIndex - 1 + NUM_OPERATIONS) % NUM_OPERATIONS;
  else if (key == 'R')
    menuIndex = (menuIndex + 1) % NUM_OPERATIONS;
  else if (key == 'E')
    calcState = WAIT_OPERAND1;
  else if (key == '=') {
    if (binaryOpsMask & (1UL << menuIndex))
      calcState = WAIT_OPERAND2;
    else {
      result = calculate();
      calcState = SHOW_RESULT;
    }
  }
  updateDisplay();
}

// Atualiza a tela do LCD de acordo com o estado atual
void updateDisplay() {
  lcdClear();
  switch (calcState) {
    case WAIT_OPERAND1:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num A"));  // Título modificado
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case SELECT_OPERATION:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Select"));  // Título modificado
      lcdSetCursor(0, 1);
      lcdPrintOperation(menuIndex);
      break;
    case WAIT_OPERAND2:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num B"));  // Título modificado
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case SHOW_RESULT:
      lcdSetCursor(0, 0);
      lcdPrintF(F("Result:"));
      lcdSetCursor(0, 1);
      {
        char resBuf[16];
        dtostrf(result, 0, 2, resBuf);
        lcdPrint(resBuf);
      }
      break;
    default:
      break;
  }
}

// Reseta a calculadora para o estado inicial
void resetCalculator() {
  calcState = WAIT_OPERAND1;
  inputBuffer[0] = '\0';
  inputLength = 0;
  hasDecimalPoint = false;
  isNegative = false;
  operand1 = operand2 = result = 0.0;
  menuIndex = 0;
  updateDisplay();
}

// --- Arduino Setup and Loop ---

void setup() {
  lcdInit();
  keypadInit();
  lcdClear();
  // Tela inicial removida – inicia direto no estado de entrada do primeiro operando
  resetCalculator();
}

void loop() {
  char key = getKey();
  if (!key)
    return;
  switch (calcState) {
    case SHOW_RESULT:
      if (key == '=')
        resetCalculator();
      break;
    case WAIT_OPERAND1:
    case WAIT_OPERAND2:
      processOperandKey(key);
      break;
    case SELECT_OPERATION:
      processOperationKey(key);
      break;
    default:
      break;
  }
}

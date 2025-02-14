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

const char opStr[] PROGMEM = "+,-,*,/,sqrt,sin,cos,tan,log,pow,fact,exp,log10,log2,cbrt,%,ax+b=c,ax^2+bx+c=d";

// --- LCD Functions ---
static inline void lcdSendNibble(uint8_t nibble) {
  PORTC = (PORTC & ~((1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5)))
          | ((nibble & 0x0F) << 2);
  PORTD |= LCD_E_MASK;
  delayMicroseconds(1);
  PORTD &= ~LCD_E_MASK;
  delayMicroseconds(80);
}

static inline void lcdCommand(uint8_t cmd) {
  PORTC &= ~((1 << PC1) | (1 << PC0));
  lcdSendNibble(cmd >> 4);
  lcdSendNibble(cmd & 0x0F);
  delay(2);
}

static inline void lcdWriteData(uint8_t data) {
  PORTC = (PORTC & ~((1 << PC1) | (1 << PC0))) | (1 << PC1);
  lcdSendNibble(data >> 4);
  lcdSendNibble(data & 0x0F);
  delay(2);
}

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

void lcdPrintF(const __FlashStringHelper *str) {
  PGM_P p = (PGM_P)str;
  char c;
  while ((c = pgm_read_byte(p++)))
    lcdWriteData(c);
}

void lcdPrintOperation(uint8_t index) {
  uint8_t currentIndex = 0;
  uint16_t i = 0;
  char c;
  while (true) {
    c = pgm_read_byte_near(opStr + i);
    if (c == '\0') break;
    if (currentIndex == index) {
      while (c != ',' && c != '\0') {
        lcdWriteData(c);
        i++;
        c = pgm_read_byte_near(opStr + i);
      }
      break;
    } else {
      while (c != ',' && c != '\0') {
        i++;
        c = pgm_read_byte_near(opStr + i);
      }
      if (c == ',') {
        currentIndex++;
        i++;
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

void keypadInit() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }
}

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
  WAIT_OPERAND2,
  WAIT_OPERAND3,
  WAIT_OPERAND4,
  SELECT_OPERATION,
  SHOW_RESULT
};
CalcState calcState = WAIT_OPERAND1;

char inputBuffer[17] = "";
uint8_t inputLength = 0;
bool hasDecimalPoint = false;
bool isNegative = false;

float operand1 = 0.0, operand2 = 0.0, operand3 = 0.0, operand4 = 0.0;
float result = 0.0;
uint8_t menuIndex = 0;
// Foram definidas 18 operações após a remoção das funções indesejadas
const uint8_t NUM_OPERATIONS = 18;
uint8_t currentOperandIndex = 1;

int getRequiredOperands(uint8_t opIndex) {
  switch(opIndex) {
    case 16: return 3; // "lin": ax+b=c
    case 17: return 4; // "quad": ax^2+bx+c=d
    default:
      // Operações que exigem 2 operandos: +, -, *, /, pow e %  
      if(opIndex == 0 || opIndex == 1 || opIndex == 2 || opIndex == 3 ||
         opIndex == 9 || opIndex == 15)
        return 2;
      else
        return 1;
  }
}

float factorial(float n) {
  if (n <= 1.0)
    return 1.0;
  float f = 1.0;
  for (int i = 2; i <= (int)n; i++)
    f *= i;
  return f;
}

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
      res = cbrt(operand1);
      break;
    case 15:
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
    case 16: // Linear: ax+b=c
      if (operand1 != 0.0)
        res = (operand3 - operand2) / operand1;
      else {
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintF(F("Error: a=0"));
        delay(2000);
        resetCalculator();
        return 0.0;
      }
      break;
    case 17: { // Quadrática: ax^2+bx+c=d
      if (operand1 != 0.0) {
        float disc = operand2 * operand2 - 4 * operand1 * (operand3 - operand4);
        if (disc < 0) {
          lcdClear();
          lcdSetCursor(0, 0);
          lcdPrintF(F("No Real Roots"));
          delay(2000);
          resetCalculator();
          return 0.0;
        } else {
          res = (-operand2 + sqrt(disc)) / (2 * operand1);
        }
      } else {
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintF(F("Error: a=0"));
        delay(2000);
        resetCalculator();
        return 0.0;
      }
    }
      break;
    default:
      break;
  }
  return res;
}

void addCharToInput(char c) {
  if (inputLength < 16) {
    inputBuffer[inputLength++] = c;
    inputBuffer[inputLength] = '\0';
  }
}

void removeCharFromInput() {
  if (inputLength > 0) {
    inputLength--;
    inputBuffer[inputLength] = '\0';
  }
}

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
    } else {
      if (calcState != SELECT_OPERATION)
        calcState = SELECT_OPERATION;
    }
  }
  else if (key == '=') {
    float value = atof(inputBuffer);
    if (isNegative)
      value = -value;
    
    switch (currentOperandIndex) {
      case 1: operand1 = value; break;
      case 2: operand2 = value; break;
      case 3: operand3 = value; break;
      case 4: operand4 = value; break;
      default: break;
    }
    
    int req = getRequiredOperands(menuIndex);
    if (currentOperandIndex < req) {
      currentOperandIndex++;
      switch (currentOperandIndex) {
        case 2: calcState = WAIT_OPERAND2; break;
        case 3: calcState = WAIT_OPERAND3; break;
        case 4: calcState = WAIT_OPERAND4; break;
      }
    } else {
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

void processOperationKey(char key) {
  if (key == 'L')
    menuIndex = (menuIndex - 1 + NUM_OPERATIONS) % NUM_OPERATIONS;
  else if (key == 'R')
    menuIndex = (menuIndex + 1) % NUM_OPERATIONS;
  else if (key == 'E')
    calcState = WAIT_OPERAND1;
  else if (key == '=') {
    int req = getRequiredOperands(menuIndex);
    if (req == 1) {
      result = calculate();
      calcState = SHOW_RESULT;
    } else {
      currentOperandIndex = 1;
      calcState = WAIT_OPERAND1;
    }
  }
  updateDisplay();
}

void updateDisplay() {
  lcdClear();
  switch (calcState) {
    case WAIT_OPERAND1:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num A"));
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case WAIT_OPERAND2:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num B"));
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case WAIT_OPERAND3:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num C"));
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case WAIT_OPERAND4:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Num D"));
      lcdSetCursor(0, 1);
      if (isNegative)
        lcdWriteData('-');
      lcdPrint(inputBuffer);
      break;
    case SELECT_OPERATION:
      lcdSetCursor(0, 0);
      lcdPrintF(F("SciCalc - Select"));
      lcdSetCursor(0, 1);
      lcdPrintOperation(menuIndex);
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

void resetCalculator() {
  calcState = SELECT_OPERATION;
  inputBuffer[0] = '\0';
  inputLength = 0;
  hasDecimalPoint = false;
  isNegative = false;
  operand1 = operand2 = operand3 = operand4 = result = 0.0;
  currentOperandIndex = 1;
  menuIndex = 0;
  updateDisplay();
}

void setup() {
  lcdInit();
  keypadInit();
  lcdClear();
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
    case WAIT_OPERAND3:
    case WAIT_OPERAND4:
      processOperandKey(key);
      break;
    case SELECT_OPERATION:
      processOperationKey(key);
      break;
    default:
      break;
  }
}

/*
  ============================================================
   ARDUINO UNO -> PIC18F4550
   PROGRAMADOR LVP V27.2

   NUEVA FUNCION:
   B = Programa BLINK integrado en Arduino

   EL ARDUINO CONTIENE EL PROGRAMA DEL PIC
   DENTRO DEL PROPIO SKETCH.

   PIC:
   RB0 -> LED -> 330R -> GND

   CONEXIONES:
   D10 -> MCLR
   D11 -> PGD
   D12 -> PGC
   D13 -> PGM
  ============================================================
*/

#include <Arduino.h>

/* ============================================================
   PINES
   ============================================================ */

#define MCLR_PIN 10
#define PGD_PIN  11
#define PGC_PIN  12
#define PGM_PIN  13

/* ============================================================
   TIEMPOS
   ============================================================ */

#define P9_US   1000
#define P10_US  200
#define P15_US  400
#define P20_US  1
#define P11_MS  5

/* ============================================================
   BUFFER DE PROGRAMACION
   ============================================================ */

uint8_t writeBuffer[32];

/* ============================================================
   PROGRAMA BLINK DEL PIC
   ============================================================

   El programa comienza en 0x0000.

   0x0000:
       GOTO START

   START:
       OSCCON = 0x72
       ADCON1 = 0x0F
       TRISB  = 0x00

       RB0 ON
       DELAY
       RB0 OFF
       DELAY
       REPETIR

   Se utilizan 80 bytes.
   Por tanto:

       ROW 0 = 0x0000 - 0x001F
       ROW 1 = 0x0020 - 0x003F
       ROW 2 = 0x0040 - 0x004F

   Los bytes están en orden LITTLE-ENDIAN,
   porque el PIC18 almacena cada instruction word
   de 16 bits en dos bytes.

   ============================================================ */

const uint8_t blinkProgram[] =
{
  /* 0000 */
  0x0B, 0xEF,       // GOTO 0x0016
  0x00, 0xF0,       // segundo word GOTO

  /* 0004 */
  0x00, 0x00,       // NOP
  0x00, 0x00,       // NOP

  /* 0008 */
  0x72, 0x0E,       // MOVLW 0x72
  0xD3, 0x6E,       // MOVWF OSCCON

  /* 000C */
  0x0F, 0x0E,       // MOVLW 0x0F
  0xC1, 0x6E,       // MOVWF ADCON1

  /* 0010 */
  0x00, 0x0E,       // MOVLW 0x00
  0x93, 0x6E,       // MOVWF TRISB

  /* 0014 */
  0x8A, 0x90,       // BCF LATB,0

  /* 0016 */
  0x8A, 0x80,       // BSF LATB,0

  /* 0018 */
  0x18, 0xEC,       // CALL 0x0030
  0x00, 0xF0,       // segundo word CALL

  /* 001C */
  0x8A, 0x90,       // BCF LATB,0

  /* 001E */
  0x18, 0xEC,       // CALL 0x0030
  0x00, 0xF0,       // segundo word CALL

  /* 0022 */
  0x0B, 0xEF,       // GOTO 0x0016
  0x00, 0xF0,       // segundo word

  /* 0026 */
  0x00, 0x00,       // NOP
  0x00, 0x00,       // NOP
  0x00, 0x00,       // NOP
  0x00, 0x00,       // NOP
  0x00, 0x00,       // NOP

  /* 0030 */
  0x04, 0x0E,       // MOVLW 0x04
  0x20, 0x6E,       // MOVWF 0x20

  /* 0034 */
  0xFF, 0x0E,       // MOVLW 0xFF
  0x21, 0x6E,       // MOVWF 0x21

  /* 0038 */
  0xFF, 0x0E,       // MOVLW 0xFF
  0x22, 0x6E,       // MOVWF 0x22

  /* 003C */
  0x22, 0xB0,       // DECFSZ 0x22,F
  0x1E, 0xEF,       // GOTO 0x003C
  0x00, 0xF0,       // segundo word

  /* 0042 */
  0x21, 0xB0,       // DECFSZ 0x21,F
  0x1C, 0xEF,       // GOTO 0x0038
  0x00, 0xF0,       // segundo word

  /* 0048 */
  0x20, 0xB0,       // DECFSZ 0x20,F
  0x1A, 0xEF,       // GOTO 0x0034
  0x00, 0xF0,       // segundo word

  /* 004E */
  0x12, 0x00        // RETURN
};

const uint8_t xc8Program[] =
{
    0x00,0x00,0x93,0x90,0x8A,0x80,0x06,0x0E,
    0x02,0x6E,0x13,0x0E,0x01,0x6E,0xAE,0x0E,

    0xE8,0x2E,0xFE,0xD7,0x01,0x2E,0xFC,0xD7,
    0x02,0x2E,0xFA,0xD7,0x8A,0x90,0x06,0x0E,

    0x02,0x6E,0x13,0x0E,0x01,0x6E,0xAE,0x0E,
    0xE8,0x2E,0xFE,0xD7,0x01,0x2E,0xFC,0xD7,

    0x02,0x2E,0xFA,0xD7,0x02,0xEF,0x04,0xF0,
    0xFE,0xEF,0x3F,0xF0,0x00,0x01,0x01,0xEF,

    0x04,0xF0
};
/* ============================================================
   PROTOTIPOS
   ============================================================ */

void enterLVP();
void exitLVP();

void send4bitcommand(uint8_t data);
void send16bit(uint16_t data);

uint8_t readFlash(uint8_t usb, uint8_t msb, uint8_t lsb);
uint16_t readDeviceID();

void eraseAll();

void clearBuffer();
void loadWriteBuffer(uint32_t address);
void programFlash();

void test1234();
void testBlink();

void commandDeviceID();
void commandErase();
void commandRead(uint32_t address);

void serialMenu();

bool programBlinkRow(uint32_t address);
bool verifyBlinkRow(uint32_t address);

uint8_t hexValue(char c);

void printHex8(uint8_t v);
void printHex16(uint16_t v);
void printHex32(uint32_t v);

/* ============================================================
   SETUP
   ============================================================ */

void setup()
{
  Serial.begin(115200);

  pinMode(MCLR_PIN, OUTPUT);
  pinMode(PGD_PIN, OUTPUT);
  pinMode(PGC_PIN, OUTPUT);
  pinMode(PGM_PIN, OUTPUT);

  digitalWrite(MCLR_PIN, LOW);
  digitalWrite(PGD_PIN, LOW);
  digitalWrite(PGC_PIN, LOW);
  digitalWrite(PGM_PIN, LOW);

  clearBuffer();

  Serial.println();
  Serial.println(F("=========================================="));
  Serial.println(F(" ARDUINO UNO -> PIC18F4550"));
  Serial.println(F(" PROGRAMADOR LVP V27.2"));
  Serial.println(F(" BLINK INTEGRADO"));
  Serial.println(F("=========================================="));
  Serial.println(F("D10 -> MCLR"));
  Serial.println(F("D11 -> PGD"));
  Serial.println(F("D12 -> PGC"));
  Serial.println(F("D13 -> PGM"));
  Serial.println();

  Serial.println(F("COMANDOS:"));
  Serial.println(F("D              = Device ID"));
  Serial.println(F("E              = Erase"));
  Serial.println(F("T              = prueba 0x1234"));
  Serial.println(F("B              = programar BLINK"));
  Serial.println(F("R 000000       = leer"));
  Serial.println();

  Serial.println(F("Arduino iniciado."));
  Serial.println(F("Serial = 115200"));
  Serial.println();

  Serial.println(F("Escribe B para programar el BLINK."));
}

/* ============================================================
   LOOP
   ============================================================ */

void loop()
{
  if (!Serial.available())
    return;

  char c = Serial.read();

  if (c == '\r' || c == '\n')
    return;

  /* ==========================================================
     TEST 1234
     ========================================================== */

  if (c == 'T' || c == 't')
  {
    test1234();
    serialMenu();
    return;
  }

  /* ==========================================================
     DEVICE ID
     ========================================================== */

  if (c == 'D' || c == 'd')
  {
    commandDeviceID();
    serialMenu();
    return;
  }

  /* ==========================================================
     ERASE
     ========================================================== */

  if (c == 'E' || c == 'e')
  {
    commandErase();
    serialMenu();
    return;
  }

  /* ==========================================================
     BLINK
     ========================================================== */

  if (c == 'B' || c == 'b')
  {
    testBlink();
    serialMenu();
    return;
  }

  /* ==========================================================
     READ
     ========================================================== */

  if (c == 'R' || c == 'r')
  {
    uint32_t address = 0;
    uint8_t digits = 0;

    while (digits < 6)
    {
      while (!Serial.available());

      char x = Serial.read();

      if (x == '\r' || x == '\n')
        break;

      if (x == ' ')
        continue;

      uint8_t v = hexValue(x);

      if (v == 0xFF)
        break;

      address = (address << 4) | v;
      digits++;
    }

    while (Serial.available())
    {
      char x = Serial.read();

      if (x == '\n')
        break;
    }

    if (digits == 6)
      commandRead(address);
    else
      Serial.println(F("ERROR: usa R 000000"));

    serialMenu();
    return;
  }
}

/* ============================================================
   MENU
   ============================================================ */

void serialMenu()
{
  Serial.println();
  Serial.println(F("------------------------------------------"));
  Serial.println(F("Escribe:"));
  Serial.println(F("T = prueba 1234"));
  Serial.println(F("D = Device ID"));
  Serial.println(F("E = Erase"));
  Serial.println(F("B = programar BLINK"));
  Serial.println(F("R 000000 = leer"));
  Serial.println(F("------------------------------------------"));
}

/* ============================================================
   ENTRADA LVP
   ============================================================ */

void enterLVP()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ENTRADA LVP"));
  Serial.println(F("================================"));

  digitalWrite(PGC_PIN, LOW);
  digitalWrite(PGD_PIN, LOW);
  digitalWrite(PGM_PIN, HIGH);
  digitalWrite(MCLR_PIN, LOW);

  Serial.println(F("PGC = LOW"));
  Serial.println(F("PGD = LOW"));
  Serial.println(F("PGM = HIGH"));
  Serial.println(F("MCLR = LOW"));
  Serial.println(F("ENVIANDO SECUENCIA MCHP"));

  delay(1);

  uint8_t seq[4] =
  {
    0x4D,
    0x43,
    0x48,
    0x50
  };

  for (uint8_t j = 0; j < 4; j++)
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      if (seq[j] & (0x80 >> i))
        digitalWrite(PGD_PIN, HIGH);
      else
        digitalWrite(PGD_PIN, LOW);

      digitalWrite(PGC_PIN, HIGH);
      digitalWrite(PGC_PIN, LOW);
    }
  }

  delayMicroseconds(P20_US);

  digitalWrite(MCLR_PIN, HIGH);

  delayMicroseconds(P15_US);

  Serial.println(F("MCLR = HIGH"));
  Serial.println(F("LVP ACTIVADO"));
}

/* ============================================================
   SALIDA LVP
   ============================================================ */

void exitLVP()
{
  digitalWrite(PGM_PIN, LOW);
  digitalWrite(MCLR_PIN, LOW);
  digitalWrite(PGC_PIN, LOW);
  digitalWrite(PGD_PIN, LOW);

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("SALIENDO DE LVP"));
  Serial.println(F("================================"));

  Serial.println(F("PGM = LOW"));
  Serial.println(F("MCLR = LOW"));
  Serial.println(F("LVP TERMINADO"));
}

/* ============================================================
   DEVICE ID
   ============================================================ */

uint16_t readDeviceID()
{
  uint8_t id1;
  uint8_t id2;

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("DEVICE ID"));
  Serial.println(F("================================"));

  Serial.println(F("TBLPTR = 0x3FFFFE"));

  id1 = readFlash(0x3F, 0xFF, 0xFE);

  Serial.print(F("DEVID1 = 0x"));
  printHex8(id1);
  Serial.println();

  Serial.println(F("TBLPTR = 0x3FFFFF"));

  id2 = readFlash(0x3F, 0xFF, 0xFF);

  Serial.print(F("DEVID2 = 0x"));
  printHex8(id2);
  Serial.println();

  uint16_t id =
    ((uint16_t)id2 << 8) |
    id1;

  Serial.print(F("DEVICE ID RAW = 0x"));
  printHex16(id);
  Serial.println();

  return id;
}

/* ============================================================
   DEVICE ID COMMAND
   ============================================================ */

void commandDeviceID()
{
  enterLVP();

  uint16_t id = readDeviceID();

  if (id == 0x1207)
    Serial.println(F("DEVICE ID: OK"));
  else
    Serial.println(F("DEVICE ID: FALLA"));

  exitLVP();
}

/* ============================================================
   READ FLASH
   ============================================================ */

uint8_t readFlash(uint8_t usb, uint8_t msb, uint8_t lsb)
{
  uint8_t value = 0;

  pinMode(PGD_PIN, OUTPUT);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | usb);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | msb);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | lsb);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  send4bitcommand(0b1000);

  pinMode(PGD_PIN, INPUT);

  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }

  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(PGC_PIN, HIGH);

    if (digitalRead(PGD_PIN))
      value |= (1 << i);

    digitalWrite(PGC_PIN, LOW);
  }

  pinMode(PGD_PIN, OUTPUT);
  digitalWrite(PGD_PIN, LOW);

  return value;
}

/* ============================================================
   READ COMMAND
   ============================================================ */

void commandRead(uint32_t address)
{
  Serial.println();
  Serial.println(F("################################"));
  Serial.println(F(" READ FLASH"));
  Serial.println(F("################################"));

  Serial.print(F("DIRECCION = 0x"));
  printHex32(address);
  Serial.println();

  enterLVP();

  uint8_t low =
    readFlash(
      (address >> 16) & 0xFF,
      (address >> 8) & 0xFF,
      address & 0xFF
    );

  address++;

  uint8_t high =
    readFlash(
      (address >> 16) & 0xFF,
      (address >> 8) & 0xFF,
      address & 0xFF
    );

  uint16_t word =
    ((uint16_t)high << 8) |
    low;

  Serial.println();
  Serial.println(F("RESULTADO"));

  Serial.print(F("LOW  = 0x"));
  printHex8(low);
  Serial.println();

  Serial.print(F("HIGH = 0x"));
  printHex8(high);
  Serial.println();

  Serial.print(F("WORD = 0x"));
  printHex16(word);
  Serial.println();

  exitLVP();
}

/* ============================================================
   ERASE ALL
   ============================================================ */

void eraseAll()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ERASE ALL"));
  Serial.println(F("================================"));

  send4bitcommand(0b0000);
  send16bit(0x0E3C);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  send4bitcommand(0b0000);
  send16bit(0x0E00);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  send4bitcommand(0b0000);
  send16bit(0x0E05);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  send4bitcommand(0b1100);
  send16bit(0x3F3F);

  send4bitcommand(0b0000);
  send16bit(0x0E3C);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  send4bitcommand(0b0000);
  send16bit(0x0E00);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  send4bitcommand(0b0000);
  send16bit(0x0E04);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  send4bitcommand(0b1100);
  send16bit(0x8F8F);

  send4bitcommand(0b0000);
  send16bit(0x0000);

  delay(2);

  send4bitcommand(0b0000);
  send16bit(0x0000);

  delay(2);

  digitalWrite(PGD_PIN, LOW);

  delay(P11_MS);

  digitalWrite(PGD_PIN, HIGH);

  Serial.println(F("ERASE TERMINADO"));
}

/* ============================================================
   ERASE COMMAND
   ============================================================ */

void commandErase()
{
  enterLVP();

  uint16_t id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println(F("ERROR: DEVICE ID"));
    exitLVP();
    return;
  }

  eraseAll();

  uint8_t value =
    readFlash(0x00, 0x00, 0x00);

  Serial.print(F("ERASE READ = 0x"));
  printHex8(value);
  Serial.println();

  if (value == 0xFF)
    Serial.println(F("ERASE VERIFY: OK"));
  else
    Serial.println(F("ERASE VERIFY: FALLA"));

  exitLVP();
}

/* ============================================================
   BUFFER
   ============================================================ */

void clearBuffer()
{
  for (uint8_t i = 0; i < 32; i++)
    writeBuffer[i] = 0xFF;
}

/* ============================================================
   LOAD WRITE BUFFER
   ============================================================ */

void loadWriteBuffer(uint32_t address)
{
  uint8_t usb =
    (address >> 16) & 0xFF;

  uint8_t msb =
    (address >> 8) & 0xFF;

  uint8_t lsb =
    address & 0xFF;

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("LOAD WRITE BUFFER"));
  Serial.println(F("================================"));

  Serial.print(F("TBLPTR = 0x"));
  printHex32(address);
  Serial.println();

  send4bitcommand(0b0000);
  send16bit(0x8EA6);

  send4bitcommand(0b0000);
  send16bit(0x9CA6);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | usb);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | msb);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  send4bitcommand(0b0000);
  send16bit(0x0E00 | lsb);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  for (uint8_t i = 0; i < 15; i++)
  {
    uint16_t word =
      ((uint16_t)writeBuffer[(i * 2) + 1] << 8) |
      writeBuffer[i * 2];

    Serial.print(F("PAIR "));
    Serial.print(i);

    Serial.print(F(" ADDR 0x"));
    printHex8(i * 2);

    Serial.print(F(" DATA 0x"));
    printHex16(word);
    Serial.println();

    send4bitcommand(0b1101);
    send16bit(word);
  }

  uint16_t lastWord =
    ((uint16_t)writeBuffer[31] << 8) |
    writeBuffer[30];

  Serial.println(F("COMANDO FINAL = 1111"));

  Serial.print(F("ULTIMO PAR = 0x"));
  printHex16(lastWord);
  Serial.println();

  send4bitcommand(0b1111);
  send16bit(lastWord);

  Serial.println(F("ULTIMO PAR ENVIADO"));
  Serial.println(F("32 BYTES TRANSMITIDOS"));
}

/* ============================================================
   PROGRAM FLASH
   ============================================================ */

void programFlash()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("PROGRAM FLASH"));
  Serial.println(F("================================"));

  Serial.println(F("COMANDO = 1111"));
  Serial.println(F("INICIO PROGRAMMING"));

  digitalWrite(PGD_PIN, LOW);

  for (uint8_t i = 0; i < 3; i++)
  {
    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }

  Serial.println(F("NOP x3"));

  digitalWrite(PGC_PIN, HIGH);

  Serial.println(F("P9 START"));

  delayMicroseconds(P9_US);

  digitalWrite(PGC_PIN, LOW);

  Serial.println(F("P9 END"));

  delayMicroseconds(P10_US);

  Serial.println(F("P10 END"));

  send16bit(0x0000);

  Serial.println(F("PROGRAMMING SEQUENCE TERMINADA"));
}

/* ============================================================
   TEST 1234
   ============================================================ */

void test1234()
{
  Serial.println();
  Serial.println(F("########################################"));
  Serial.println(F(" V27.2 TEST 0x1234"));
  Serial.println(F("########################################"));

  enterLVP();

  uint16_t id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println(F("DEVICE ID INCORRECTO"));
    exitLVP();
    return;
  }

  Serial.println(F("DEVICE ID: OK"));

  eraseAll();

  uint8_t erased =
    readFlash(0x00, 0x00, 0x00);

  Serial.print(F("FLASH[0] AFTER ERASE = 0x"));
  printHex8(erased);
  Serial.println();

  if (erased != 0xFF)
  {
    Serial.println(F("ERASE VERIFY: FALLA"));
    exitLVP();
    return;
  }

  Serial.println(F("ERASE VERIFY: OK"));

  clearBuffer();

writeBuffer[0] = 0x34;
writeBuffer[1] = 0x12;


  loadWriteBuffer(0x000800);

  programFlash();

  uint8_t low =
    readFlash(0x00, 0x08, 0x00);

  uint8_t high =
    readFlash(0x00, 0x08, 0x01);

  uint16_t word =
    ((uint16_t)high << 8) |
    low;

  Serial.print(F("WORD = 0x"));
  printHex16(word);
  Serial.println();

  if (word == 0x1234)
    Serial.println(F("TEST 1234 CORRECTO"));
  else
    Serial.println(F("TEST 1234 FALLIDO"));

  exitLVP();
}

/* ============================================================
   PROGRAMAR BLINK
   ============================================================ */

void testBlink()
{
  const uint16_t programSize =
    sizeof(xc8Program);

  const uint8_t rows =
    (programSize + 31) / 32;

  Serial.println();
  Serial.println(F("########################################"));
  Serial.println(F(" V27.2 TEST BLINK"));
  Serial.println(F("########################################"));

  Serial.print(F("TAMANO PROGRAMA = "));
  Serial.print(programSize);
  Serial.println(F(" BYTES"));

  Serial.print(F("ROWS = "));
  Serial.println(rows);

  Serial.println();
  Serial.println(F("LED EN RB0"));
  Serial.println(F("RB0 = PIN 33 DEL PIC18F4550"));
  Serial.println();

  /* ==========================================================
     ENTRAR LVP
     ========================================================== */

  enterLVP();

  /* ==========================================================
     DEVICE ID
     ========================================================== */

  uint16_t id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println(F("ERROR: DEVICE ID"));
    exitLVP();
    return;
  }

  Serial.println(F("DEVICE ID: OK"));

  /* ==========================================================
     ERASE
     ========================================================== */

  eraseAll();

  uint8_t erased =
    readFlash(0x00, 0x00, 0x00);

  Serial.print(F("ERASE VERIFY = 0x"));
  printHex8(erased);
  Serial.println();

  if (erased != 0xFF)
  {
    Serial.println(F("ERASE VERIFY: FALLA"));
    exitLVP();
    return;
  }

  Serial.println(F("ERASE VERIFY: OK"));

  /* ==========================================================
     PROGRAMAR CADA ROW
     ========================================================== */

  bool globalOK = true;

  for (uint8_t row = 0; row < rows; row++)
  {
    uint32_t address =
    0x0800UL + ((uint32_t)row * 32UL);

    Serial.println();
    Serial.println(F("================================"));
    Serial.print(F("PROGRAM BLINK ROW "));
    Serial.println(row);
    Serial.println(F("================================"));

    Serial.print(F("ADDRESS = 0x"));
    printHex32(address);
    Serial.println();

    clearBuffer();

    /* Cargar 32 bytes */

for (uint8_t i = 0; i < 32; i++)
{
    uint32_t flashOffset =
        (address - 0x0800UL) + i;

    if (flashOffset < programSize)
        writeBuffer[i] = xc8Program[flashOffset];
    else
        writeBuffer[i] = 0xFF;
}
    /* Mostrar datos */
    Serial.println("DEBUG");

for(int k=0;k<16;k++)
{
    Serial.print(xc8Program[k],HEX);
    Serial.print(" ");
}

Serial.println();

    Serial.println(F("DATA:"));

    for (uint8_t i = 0; i < 32; i++)
    {
      printHex8(writeBuffer[i]);
      Serial.print(' ');

      if (i == 15)
        Serial.println();
    }

    Serial.println();

    /* ========================================================
       PROGRAMAR
       ======================================================== */

    loadWriteBuffer(address);

    programFlash();

    /* ========================================================
       VERIFY
       ======================================================== */

    Serial.println(F("VERIFY ROW"));

    bool rowOK = true;

    for (uint8_t i = 0; i < 32; i++)
    {
     uint32_t currentAddress =
    address + i;

uint32_t flashOffset =
    (address - 0x0800UL) + i;

uint8_t expected = 0xFF;

if (flashOffset < programSize)
    expected = xc8Program[flashOffset];

uint8_t value =
    readFlash(
      (currentAddress >> 16) & 0xFF,
      (currentAddress >> 8) & 0xFF,
      currentAddress & 0xFF
    );

if (value != expected)
{
    rowOK = false;
    globalOK = false;

    Serial.print(F("ERROR ADDR 0x"));
    printHex32(currentAddress);

    Serial.print(F(" ESP=0x"));
    printHex8(expected);

    Serial.print(F(" LEI=0x"));
    printHex8(value);

    Serial.println();
}


 
    }

    if (rowOK)
      Serial.println(F("ROW VERIFY: OK"));
    else
      Serial.println(F("ROW VERIFY: FALLA"));
  }

  /* ==========================================================
     RESULTADO
     ========================================================== */

  Serial.println();
  Serial.println(F("########################################"));

  if (globalOK)
  {
    Serial.println(F(" BLINK PROGRAMADO CORRECTAMENTE"));
    Serial.println(F(" PROGRAMACION = OK"));
    Serial.println(F(" VERIFICACION  = OK"));
  }
  else
  {
    Serial.println(F(" BLINK PROGRAMACION FALLIDA"));
  }

  Serial.println(F("########################################"));

  exitLVP();

  if (globalOK)
  {
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F(" AHORA REINICIA EL PIC"));
    Serial.println(F(" RB0 DEBE PARPADEAR"));
    Serial.println(F("========================================"));
  }
}

/* ============================================================
   SEND 4 BITS
   ============================================================ */

void send4bitcommand(uint8_t data)
{
  pinMode(PGD_PIN, OUTPUT);

  for (uint8_t i = 0; i < 4; i++)
  {
    if (data & (1 << i))
      digitalWrite(PGD_PIN, HIGH);
    else
      digitalWrite(PGD_PIN, LOW);

    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }
}

/* ============================================================
   SEND 16 BITS
   ============================================================ */

void send16bit(uint16_t data)
{
  pinMode(PGD_PIN, OUTPUT);

  for (uint8_t i = 0; i < 16; i++)
  {
    if (data & (1UL << i))
      digitalWrite(PGD_PIN, HIGH);
    else
      digitalWrite(PGD_PIN, LOW);

    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }
}

/* ============================================================
   HEX VALUE
   ============================================================ */

uint8_t hexValue(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  return 0xFF;
}

/* ============================================================
   PRINT HEX
   ============================================================ */

void printHex8(uint8_t v)
{
  if (v < 0x10)
    Serial.print('0');

  Serial.print(v, HEX);
}

void printHex16(uint16_t v)
{
  printHex8((v >> 8) & 0xFF);
  printHex8(v & 0xFF);
}

void printHex32(uint32_t v)
{
  printHex8((v >> 24) & 0xFF);
  printHex8((v >> 16) & 0xFF);
  printHex8((v >> 8) & 0xFF);
  printHex8(v & 0xFF);
}


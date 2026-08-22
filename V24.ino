/*
   ============================================================
   ARDUINO UNO -> PIC18F4550
   PROGRAMADOR LVP V24
   Basado en arduino-as-pic18f-programmer
   ============================================================

   PINES:

   Arduino D10 -> PIC MCLR
   Arduino D11 -> PIC PGD
   Arduino D12 -> PIC PGC
   Arduino D13 -> PIC PGM

   Arduino 5V  -> PIC VDD
   Arduino GND -> PIC VSS

   COMANDO:
   T = prueba completa

   PROGRAMA:
   0x1234 en dirección 0x000000
*/

#define PIN_MCLR 10
#define PIN_PGD  11
#define PIN_PGC  12
#define PIN_PGM  13

// ------------------------------------------------------------
// TIEMPOS
// Basados en la rutina original del repositorio
// ------------------------------------------------------------

#define P5_US       1
#define P6_US       1
#define P9_US       1000
#define P10_US      200
#define P11_MS      5
#define P15_US      400
#define P18_US      1000
#define P20_US      1

// ------------------------------------------------------------
// BUFFER
// ------------------------------------------------------------

byte buffer32[32];

// ------------------------------------------------------------
// UTILIDADES
// ------------------------------------------------------------

void clockPulse()
{
  digitalWrite(PIN_PGC, HIGH);
  digitalWrite(PIN_PGC, LOW);
}

// ------------------------------------------------------------
// PGD OUTPUT
// ------------------------------------------------------------

void pgdOutput()
{
  pinMode(PIN_PGD, OUTPUT);
}

// ------------------------------------------------------------
// PGD INPUT
// ------------------------------------------------------------

void pgdInput()
{
  pinMode(PIN_PGD, INPUT);
}

// ------------------------------------------------------------
// COMANDO 4 BITS
// ------------------------------------------------------------

void send4bitCommand(byte data)
{
  pgdOutput();

  for (byte i = 0; i < 4; i++)
  {
    if (data & (1 << i))
      digitalWrite(PIN_PGD, HIGH);
    else
      digitalWrite(PIN_PGD, LOW);

    clockPulse();
  }
}

// ------------------------------------------------------------
// BYTE MSB FIRST
// ------------------------------------------------------------

void sendByteMSB(byte data)
{
  pgdOutput();

  for (byte i = 0; i < 8; i++)
  {
    if (data & (0x80 >> i))
      digitalWrite(PIN_PGD, HIGH);
    else
      digitalWrite(PIN_PGD, LOW);

    clockPulse();
  }
}

// ------------------------------------------------------------
// BYTE LSB FIRST
// ------------------------------------------------------------

byte readByteLSB()
{
  byte value = 0;

  pgdInput();
  digitalWrite(PIN_PGD, LOW);

  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(PIN_PGC, HIGH);

    if (digitalRead(PIN_PGD))
      value |= (1 << i);

    digitalWrite(PIN_PGC, LOW);
  }

  return value;
}

// ------------------------------------------------------------
// 16 BITS LSB FIRST
// ------------------------------------------------------------

void send16bit(unsigned int data)
{
  pgdOutput();

  for (byte i = 0; i < 16; i++)
  {
    if (data & (1 << i))
      digitalWrite(PIN_PGD, HIGH);
    else
      digitalWrite(PIN_PGD, LOW);

    clockPulse();
  }
}

// ------------------------------------------------------------
// HEX
// ------------------------------------------------------------

void printHex8(byte b)
{
  if (b < 0x10)
    Serial.print('0');

  Serial.print(b, HEX);
}

void printHex16(unsigned int v)
{
  printHex8((byte)(v >> 8));
  printHex8((byte)v);
}

// ------------------------------------------------------------
// BUFFER FF
// ------------------------------------------------------------

void clearBuffer()
{
  for (byte i = 0; i < 32; i++)
    buffer32[i] = 0xFF;
}

// ------------------------------------------------------------
// BUFFER DE PRUEBA
// ------------------------------------------------------------

void prepareTestBuffer()
{
  clearBuffer();

  // PIC18F4550
  // palabra 0x1234
  // byte bajo primero

  buffer32[0] = 0x34;
  buffer32[1] = 0x12;
}

// ------------------------------------------------------------
// ENTRADA LVP
// ------------------------------------------------------------

void enterLVP()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ENTRADA LVP"));
  Serial.println(F("================================"));

  digitalWrite(PIN_PGC, LOW);
  digitalWrite(PIN_PGD, LOW);
  digitalWrite(PIN_PGM, HIGH);
  digitalWrite(PIN_MCLR, LOW);

  Serial.println(F("PGC = LOW"));
  Serial.println(F("PGD = LOW"));
  Serial.println(F("PGM = HIGH"));
  Serial.println(F("MCLR = LOW"));

  Serial.println(F("ENVIANDO SECUENCIA MCHP"));

  /*
     Secuencia del repositorio:

     0x4D 43 48 50
     "MCHP"
  */

  sendByteMSB(0x4D);
  sendByteMSB(0x43);
  sendByteMSB(0x48);
  sendByteMSB(0x50);

  delayMicroseconds(P20_US);

  digitalWrite(PIN_MCLR, HIGH);

  delayMicroseconds(P15_US);

  Serial.println(F("MCLR = HIGH"));
  Serial.println(F("LVP ACTIVADO"));
}

// ------------------------------------------------------------
// SALIR LVP
// ------------------------------------------------------------

void exitLVP()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("SALIENDO DE LVP"));
  Serial.println(F("================================"));

  digitalWrite(PIN_PGM, LOW);
  digitalWrite(PIN_MCLR, LOW);
  digitalWrite(PIN_PGC, LOW);
  digitalWrite(PIN_PGD, LOW);

  Serial.println(F("PGM = LOW"));
  Serial.println(F("MCLR = LOW"));
  Serial.println(F("LVP TERMINADO"));
}

// ------------------------------------------------------------
// READ FLASH
// MISMA LOGICA DEL REPOSITORIO
// ------------------------------------------------------------

byte readFlash(byte upper, byte high, byte low)
{
  byte value;

  pgdOutput();

  // MOVLW upper
  send4bitCommand(0b0000);
  send16bit(0x0E00 | upper);

  // MOVWF TBLPTRU
  send4bitCommand(0b0000);
  send16bit(0x6EF8);

  // MOVLW high
  send4bitCommand(0b0000);
  send16bit(0x0E00 | high);

  // MOVWF TBLPTRH
  send4bitCommand(0b0000);
  send16bit(0x6EF7);

  // MOVLW low
  send4bitCommand(0b0000);
  send16bit(0x0E00 | low);

  // MOVWF TBLPTRL
  send4bitCommand(0b0000);
  send16bit(0x6EF6);

  // TABLE READ
  send4bitCommand(0b1000);

  pgdInput();
  digitalWrite(PIN_PGD, LOW);

  // dummy byte
  for (byte i = 0; i < 8; i++)
    clockPulse();

  // data byte
  value = 0;

  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(PIN_PGC, HIGH);

    if (digitalRead(PIN_PGD))
      value |= (1 << i);

    digitalWrite(PIN_PGC, LOW);
  }

  return value;
}

// ------------------------------------------------------------
// DEVICE ID
// ------------------------------------------------------------

unsigned int readDeviceID()
{
  byte id1;
  byte id2;

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

  unsigned int id = ((unsigned int)id2 << 8) | id1;

  Serial.print(F("DEVICE ID RAW = 0x"));
  printHex16(id);
  Serial.println();

  return id;
}

// ------------------------------------------------------------
// ERASE ALL
// BASADO EN EL REPOSITORIO
// ------------------------------------------------------------

void eraseAll()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ERASE ALL"));
  Serial.println(F("================================"));

  // TBLPTRU = 0x3F
  send4bitCommand(0b0000);
  send16bit(0x0E3C);

  send4bitCommand(0b0000);
  send16bit(0x6EF8);

  // TBLPTRH = 0x00
  send4bitCommand(0b0000);
  send16bit(0x0E00);

  send4bitCommand(0b0000);
  send16bit(0x6EF7);

  // TBLPTRL = 0x05
  send4bitCommand(0b0000);
  send16bit(0x0E05);

  send4bitCommand(0b0000);
  send16bit(0x6EF6);

  // ERASE
  send4bitCommand(0b1100);
  send16bit(0x3F3F);

  // segunda secuencia

  send4bitCommand(0b0000);
  send16bit(0x0E3C);

  send4bitCommand(0b0000);
  send16bit(0x6EF8);

  send4bitCommand(0b0000);
  send16bit(0x0E00);

  send4bitCommand(0b0000);
  send16bit(0x6EF7);

  send4bitCommand(0b0000);
  send16bit(0x0E04);

  send4bitCommand(0b0000);
  send16bit(0x6EF6);

  send4bitCommand(0b1100);
  send16bit(0x8F8F);

  // NOP

  send4bitCommand(0b0000);
  send16bit(0x0000);

  delay(2);

  send4bitCommand(0b0000);
  send16bit(0x0000);

  delay(2);

  digitalWrite(PIN_PGD, LOW);

  delay(P11_MS);

  digitalWrite(PIN_PGD, HIGH);

  Serial.println(F("ERASE TERMINADO"));
}

// ------------------------------------------------------------
// CARGAR WRITE BUFFER
// EXACTAMENTE LA ESTRUCTURA DEL REPOSITORIO
// ------------------------------------------------------------

void loadWriteBuffer()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 4: LOAD WRITE BUFFER"));
  Serial.println(F("================================"));

  // EEPGD = 1
  send4bitCommand(0b0000);
  send16bit(0x8EA6);

  // CFGS = 0
  send4bitCommand(0b0000);
  send16bit(0x9CA6);

  // TBLPTRU = 0
  send4bitCommand(0b0000);
  send16bit(0x0E00);

  send4bitCommand(0b0000);
  send16bit(0x6EF8);

  // TBLPTRH = 0
  send4bitCommand(0b0000);
  send16bit(0x0E00);

  send4bitCommand(0b0000);
  send16bit(0x6EF7);

  // TBLPTRL = 0
  send4bitCommand(0b0000);
  send16bit(0x0E00);

  send4bitCommand(0b0000);
  send16bit(0x6EF6);

  Serial.println(F("TBLPTR = 0x000000"));

  // 15 pares = 30 bytes
  for (byte i = 0; i < 15; i++)
  {
    unsigned int word =
      ((unsigned int)buffer32[(2 * i) + 1] << 8) |
      buffer32[(2 * i)];

    Serial.print(F("PAIR "));
    Serial.print(i);
    Serial.print(F(" ADDR 0x"));

    byte addr = i * 2;

    printHex8(addr);

    Serial.print(F(" DATA 0x"));
    printHex16(word);
    Serial.println();

    send4bitCommand(0b1101);
    send16bit(word);
  }

  // ----------------------------------------------------------
  // ÚLTIMO PAR
  // COMANDO 1111
  // ----------------------------------------------------------

  unsigned int lastWord =
    ((unsigned int)buffer32[31] << 8) |
    buffer32[30];

  Serial.print(F("PAIR 15 ADDR 0x1E DATA 0x"));
  printHex16(lastWord);
  Serial.println();

  Serial.println(F("COMANDO FINAL = 1111"));

  send4bitCommand(0b1111);
  send16bit(lastWord);

  Serial.println(F("ULTIMO PAR ENVIADO"));
  Serial.println(F("32 BYTES TRANSMITIDOS"));
}

// ------------------------------------------------------------
// PROGRAM FLASH
// MISMA SECUENCIA DEL REPOSITORIO
// ------------------------------------------------------------

void programFlash()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 5: PROGRAM FLASH"));
  Serial.println(F("================================"));

  Serial.println(F("COMANDO = 1111"));
  Serial.println(F("INICIO PROGRAMMING"));

  // NOP x3

  digitalWrite(PIN_PGD, LOW);

  clockPulse();
  clockPulse();
  clockPulse();

  Serial.println(F("NOP x3"));

  // P9

  digitalWrite(PIN_PGC, HIGH);

  Serial.println(F("P9 START"));

  delayMicroseconds(P9_US);

  digitalWrite(PIN_PGC, LOW);

  Serial.println(F("P9 END"));

  // P10

  delayMicroseconds(P10_US);

  Serial.println(F("P10 END"));

  // liberar secuencia

  send16bit(0x0000);

  Serial.println(F("PROGRAMMING SEQUENCE TERMINADA"));
}

// ------------------------------------------------------------
// LEER 32 BYTES
// ------------------------------------------------------------

void read32Bytes()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 6: FLASH READ"));
  Serial.println(F("================================"));

  for (byte i = 0; i < 32; i++)
  {
    byte value = readFlash(0x00, 0x00, i);

    if ((i & 0x0F) == 0)
  {
      Serial.print(F("0x"));
      if (i < 0x10) Serial.print('0');
      Serial.print(i, HEX);
      Serial.print(F(": "));
    }

    printHex8(value);
    Serial.print(' ');

    if ((i & 0x0F) == 0x0F)
      Serial.println();
  }
}

// ------------------------------------------------------------
// LEER PRIMERA PALABRA
// ------------------------------------------------------------

unsigned int readFirstWord()
{
  byte low;
  byte high;

  low = readFlash(0x00, 0x00, 0x00);
  high = readFlash(0x00, 0x00, 0x01);

  return ((unsigned int)high << 8) | low;
}

// ------------------------------------------------------------
// DIAGNOSTICO
// ------------------------------------------------------------

void verifyResult()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 7: VERIFICACION"));
  Serial.println(F("================================"));

  unsigned int value = readFirstWord();

  Serial.print(F("ESPERADO = 0x"));
  printHex16(0x1234);
  Serial.println();

  Serial.print(F("LEIDO    = 0x"));
  printHex16(value);
  Serial.println();

  if (value == 0x1234)
  {
    Serial.println();
    Serial.println(F("********************************"));
    Serial.println(F(" PROGRAMACION CORRECTA"));
    Serial.println(F("********************************"));
  }
  else
  {
    Serial.println();

    Serial.print(F("ERROR BYTE 0 ESPERADO=0x34 LEIDO=0x"));
    printHex8(value & 0xFF);
    Serial.println();

    Serial.print(F("ERROR BYTE 1 ESPERADO=0x12 LEIDO=0x"));
    printHex8(value >> 8);
    Serial.println();

    Serial.println();
    Serial.println(F("********************************"));
    Serial.println(F(" PROGRAMACION FALLIDA"));
    Serial.println(F("********************************"));

    Serial.println();
    Serial.println(F("DIAGNOSTICO V24"));
    Serial.println(F("--------------------------------"));
    Serial.println(F("DEVICE ID       : OK"));
    Serial.println(F("ERASE           : EJECUTADO"));
    Serial.println(F("WRITE BUFFER    : 32 BYTES"));
    Serial.println(F("PAIR 15         : ENVIADO"));
    Serial.println(F("CMD 1111        : ENVIADO"));
    Serial.println(F("P9              : EJECUTADO"));
    Serial.println(F("P10             : EJECUTADO"));
    Serial.println(F("FLASH VERIFY    : FALLA"));
    Serial.println();
    Serial.println(F("SIGUIENTE PUNTO A INVESTIGAR:"));
    Serial.println(F("WRITE LATCH / PROGRAM SEQUENCE"));
  }
}

// ------------------------------------------------------------
// PRUEBA COMPLETA
// ------------------------------------------------------------

void runTest()
{
  Serial.println();
  Serial.println(F("########################################"));
  Serial.println(F(" PIC18F4550 V24"));
  Serial.println(F(" DIAGNOSTICO FLASH"));
  Serial.println(F("########################################"));

  // preparar buffer
  prepareTestBuffer();

  // entrar
  enterLVP();

  // Device ID
  unsigned int id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println();
    Serial.println(F("DEVICE ID INCORRECTO"));
    Serial.println(F("NO SE CONTINUA CON ERASE/WRITE"));

    exitLVP();
    return;
  }

  Serial.println(F("DEVICE ID: OK"));

  // erase
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 2: ERASE"));
  Serial.println(F("================================"));

  eraseAll();

  // comprobar erase
  byte eraseValue = readFlash(0x00, 0x00, 0x00);

  Serial.print(F("FLASH[0x000000] AFTER ERASE = 0x"));
  printHex8(eraseValue);
  Serial.println();

  if (eraseValue == 0xFF)
    Serial.println(F("ERASE VERIFY: OK"));
  else
    Serial.println(F("ERASE VERIFY: SOSPECHOSO"));

  // buffer
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ETAPA 3: WRITE BUFFER"));
  Serial.println(F("================================"));

  Serial.println(F("BUFFER:"));

  for (byte i = 0; i < 32; i++)
  {
    printHex8(buffer32[i]);
    Serial.print(' ');

    if ((i & 0x0F) == 0x0F)
      Serial.println();
  }

  Serial.println();
  Serial.println(F("ESPERADO:"));
  Serial.println(F("ADDR 0x000000 = 0x1234"));

  // cargar
  loadWriteBuffer();

  // programar
  programFlash();

  // leer
  read32Bytes();

  // verificar
  verifyResult();

  // salir
  exitLVP();

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("PROCESO TERMINADO"));
  Serial.println(F("================================"));
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_PGC, OUTPUT);
  pinMode(PIN_PGD, OUTPUT);
  pinMode(PIN_PGM, OUTPUT);
  pinMode(PIN_MCLR, OUTPUT);

  digitalWrite(PIN_PGC, LOW);
  digitalWrite(PIN_PGD, LOW);
  digitalWrite(PIN_PGM, LOW);
  digitalWrite(PIN_MCLR, LOW);

  clearBuffer();

  Serial.println();
  Serial.println(F("=========================================="));
  Serial.println(F(" ARDUINO UNO -> PIC18F4550"));
  Serial.println(F(" PROGRAMADOR LVP V24"));
  Serial.println(F("=========================================="));

  Serial.println(F("D10 -> MCLR"));
  Serial.println(F("D11 -> PGD"));
  Serial.println(F("D12 -> PGC"));
  Serial.println(F("D13 -> PGM"));
  Serial.println();
  Serial.println(F("T = PROGRAMAR 0x1234"));
  Serial.println();
  Serial.println(F("Arduino iniciado."));
  Serial.println(F("Velocidad Serial: 115200"));
  Serial.println();
  Serial.println(F("Escribe T para comenzar."));
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------

void loop()
{
  if (Serial.available())
  {
    char c = Serial.read();

    if (c == 'T' || c == 't')
    {
      runTest();

      Serial.println();
      Serial.println(F("Escribe T para repetir."));
    }
  }
}

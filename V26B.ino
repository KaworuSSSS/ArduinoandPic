/*
  ============================================================
   ARDUINO UNO -> PIC18F4550
   PROGRAMADOR LVP V26B
   BASADO EN LA RUTINA V25 FUNCIONAL
   + RECEPTOR INTEL HEX LINEA POR LINEA

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

#define P5_US   1
#define P6_US   1
#define P9_US   1000
#define P9A_US  5000
#define P10_US  200
#define P15_US  400
#define P18_US  1000
#define P20_US  1
#define P11_MS  5

/* ============================================================
   BUFFER DE PROGRAMACION
   32 BYTES = 16 WORDS
   ============================================================ */

uint8_t writeBuffer[32];

/* ============================================================
   VARIABLES HEX
   ============================================================ */

uint8_t hexLine[80];
uint8_t hexLen = 0;

uint32_t hexBaseAddress = 0;

uint32_t rowAddress = 0;
bool rowActive = false;
bool rowDirty = false;

/* ============================================================
   PROTOTIPOS
   ============================================================ */

void enterLVP();
void exitLVP();

uint8_t readFlash(uint8_t usb, uint8_t msb, uint8_t lsb);
uint16_t readDeviceID();

void eraseAll();

void loadWriteBuffer(uint32_t address);
void programFlash();

void test1234();
void commandDeviceID();
void commandErase();
void commandRead(uint32_t address);

void printHex8(uint8_t v);
void printHex16(uint16_t v);
void printHex32(uint32_t v);

void clearBuffer();

bool receiveHexLine();
bool parseHexLine();
bool processHexRecord();
bool flushCurrentRow();

uint8_t hexValue(char c);
uint8_t hexByte(uint8_t pos);

void serialMenu();

/* ============================================================
   SETUP
   ============================================================ */

void setup()
{
  Serial.begin(115200);

  pinMode(PGC_PIN, OUTPUT);
  pinMode(PGD_PIN, OUTPUT);
  pinMode(PGM_PIN, OUTPUT);
  pinMode(MCLR_PIN, OUTPUT);

  digitalWrite(PGC_PIN, LOW);
  digitalWrite(PGD_PIN, LOW);
  digitalWrite(PGM_PIN, LOW);
  digitalWrite(MCLR_PIN, LOW);

  clearBuffer();

  Serial.println();
  Serial.println(F("=========================================="));
  Serial.println(F(" ARDUINO UNO -> PIC18F4550"));
  Serial.println(F(" PROGRAMADOR LVP V26B"));
  Serial.println(F(" INTEL HEX STREAM"));
  Serial.println(F("=========================================="));
  Serial.println(F("D10 -> MCLR"));
  Serial.println(F("D11 -> PGD"));
  Serial.println(F("D12 -> PGC"));
  Serial.println(F("D13 -> PGM"));
  Serial.println();

  Serial.println(F("COMANDOS:"));
  Serial.println(F("T              = prueba 0x1234"));
  Serial.println(F("D              = Device ID"));
  Serial.println(F("E              = Erase"));
  Serial.println(F("R 000000       = leer"));
  Serial.println(F("HEX            = recibir Intel HEX"));
  Serial.println();

  Serial.println(F("Arduino iniciado."));
  Serial.println(F("Velocidad Serial: 115200"));
  Serial.println();
  Serial.println(F("Escribe D para comprobar Device ID."));
}

/* ============================================================
   LOOP
   ============================================================ */

void loop()
{
  if (!Serial.available())
    return;

  char command = Serial.read();

  if (command == '\r' || command == '\n')
    return;

  if (command == 'T' || command == 't')
  {
    test1234();
    serialMenu();
  }
  else if (command == 'D' || command == 'd')
  {
    commandDeviceID();
    serialMenu();
  }
  else if (command == 'E' || command == 'e')
  {
    commandErase();
    serialMenu();
  }
  else if (command == 'R' || command == 'r')
  {
    /* Esperar espacio */
    while (Serial.available() == 0);

    while (Serial.available())
    {
      char c = Serial.read();

      if (c == '\r' || c == '\n')
        break;

      if (c == ' ')
        continue;

      hexLine[hexLen++] = c;

      if (hexLen >= 6)
        break;
    }

    if (hexLen >= 6)
    {
      uint32_t address = 0;

      for (uint8_t i = 0; i < 6; i++)
      {
        address <<= 4;
        address |= hexValue(hexLine[i]);
      }

      hexLen = 0;

      commandRead(address);
    }
    else
    {
      hexLen = 0;
      Serial.println(F("ERROR: R requiere direccion de 6 digitos."));
    }

    serialMenu();
  }
  else if (command == 'H' || command == 'h')
  {
    /*
      Comando HEX.
      El programa espera lineas Intel HEX.
    */

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F(" MODO INTEL HEX"));
    Serial.println(F("=========================================="));
    Serial.println(F("Envia ahora el contenido del build.hex"));
    Serial.println(F("Una linea por registro."));
    Serial.println(F("Finaliza con :00000001FF"));
    Serial.println();

    receiveHexLine();

    serialMenu();
  }
  else
  {
    /*
      Permitir recibir directamente un ':'.
    */
    if (command == ':')
    {
      Serial.println(F("HEX detectado."));
      receiveHexLine();
      serialMenu();
    }
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
  Serial.println(F("R 000000 = leer"));
  Serial.println(F("HEX = cargar build.hex"));
  Serial.println(F("------------------------------------------"));
}

/* ============================================================
   ENTRADA LVP
   ESTA ES LA SECUENCIA DE LA V25 FUNCIONAL
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

  /*
     Secuencia MCHP:
     0x4D 0x43 0x48 0x50
  */

  digitalWrite(MCLR_PIN, LOW);
  delay(1);

  uint8_t seq[4];

  seq[0] = 0x4D;
  seq[1] = 0x43;
  seq[2] = 0x48;
  seq[3] = 0x50;

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

  uint16_t id = ((uint16_t)id2 << 8) | id1;

  Serial.print(F("DEVICE ID RAW = 0x"));
  printHex16(id);
  Serial.println();

  return id;
}

/* ============================================================
   COMANDO DEVICE ID
   ============================================================ */

void commandDeviceID()
{
  enterLVP();

  uint16_t id = readDeviceID();

  if (id == 0x1207)
  {
    Serial.println(F("DEVICE ID: OK"));
  }
  else
  {
    Serial.println(F("DEVICE ID: FALLA"));
  }

  exitLVP();
}

/* ============================================================
   READ FLASH
   RUTINA BASADA EN V25
   ============================================================ */

uint8_t readFlash(uint8_t usb, uint8_t msb, uint8_t lsb)
{
  uint8_t value = 0;

  pinMode(PGD_PIN, OUTPUT);

  /*
     MOVLW usb
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | usb);

  /*
     MOVWF TBLPTRU
  */

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  /*
     MOVLW msb
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | msb);

  /*
     MOVWF TBLPTRH
  */

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  /*
     MOVLW lsb
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | lsb);

  /*
     MOVWF TBLPTRL
  */

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  /*
     TBLRD*
  */

  send4bitcommand(0b1000);

  pinMode(PGD_PIN, INPUT);
  digitalWrite(PGD_PIN, LOW);

  /*
     Dummy read
  */

  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }

  /*
     Leer byte
  */

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
   COMANDO READ
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

  uint8_t usb = (address >> 16) & 0xFF;
  uint8_t msb = (address >> 8) & 0xFF;
  uint8_t lsb = address & 0xFF;

  uint8_t low = readFlash(usb, msb, lsb);

  address++;

  usb = (address >> 16) & 0xFF;
  msb = (address >> 8) & 0xFF;
  lsb = address & 0xFF;

  uint8_t high = readFlash(usb, msb, lsb);

  uint16_t word = ((uint16_t)high << 8) | low;

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
   BASADO EN V25
   ============================================================ */

void eraseAll()
{
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ERASE ALL"));
  Serial.println(F("================================"));

  /*
     TBLPTRU = 0x3C
  */

  send4bitcommand(0b0000);
  send16bit(0x0E3C);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  /*
     TBLPTRH = 0
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  /*
     TBLPTRL = 5
  */

  send4bitcommand(0b0000);
  send16bit(0x0E05);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  /*
     ERASE
  */

  send4bitcommand(0b1100);
  send16bit(0x3F3F);

  /*
     TBLPTRU = 3C
  */

  send4bitcommand(0b0000);
  send16bit(0x0E3C);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  /*
     TBLPTRH = 0
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  /*
     TBLPTRL = 4
  */

  send4bitcommand(0b0000);
  send16bit(0x0E04);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  /*
     ERASE
  */

  send4bitcommand(0b1100);
  send16bit(0x8F8F);

  /*
     NOP
  */

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
   COMANDO ERASE
   ============================================================ */

void commandErase()
{
  enterLVP();

  uint16_t id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println(F("ERROR: DEVICE ID INCORRECTO"));
    exitLVP();
    return;
  }

  eraseAll();

  uint8_t value = readFlash(0x00, 0x00, 0x00);

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
  uint8_t usb = (address >> 16) & 0xFF;
  uint8_t msb = (address >> 8) & 0xFF;
  uint8_t lsb = address & 0xFF;

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("LOAD WRITE BUFFER"));
  Serial.println(F("================================"));

  Serial.print(F("TBLPTR = 0x"));
  printHex32(address);
  Serial.println();

  /*
     EEPGD = 1
     CFGS  = 0
  */

  send4bitcommand(0b0000);
  send16bit(0x8EA6);

  send4bitcommand(0b0000);
  send16bit(0x9CA6);

  /*
     TBLPTRU
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | usb);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  /*
     TBLPTRH
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | msb);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  /*
     TBLPTRL
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00 | lsb);

  send4bitcommand(0b0000);
  send16bit(0x6EF6);

  /*
     Primeros 15 pares
  */

  for (uint8_t i = 0; i < 15; i++)
  {
    Serial.print(F("PAIR "));
    Serial.print(i);
    Serial.print(F(" ADDR 0x"));
    printHex8(i * 2);
    Serial.print(F(" DATA 0x"));

    uint16_t word =
      ((uint16_t)writeBuffer[i * 2 + 1] << 8) |
      writeBuffer[i * 2];

    printHex16(word);
    Serial.println();

    send4bitcommand(0b1101);
    send16bit(word);
  }

  /*
     Ultimo par.
     Comando 1111 inicia programación.
  */

  uint16_t lastWord =
    ((uint16_t)writeBuffer[31] << 8) |
    writeBuffer[30];

  Serial.print(F("COMANDO FINAL = 1111"));
  Serial.println();

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

  /*
     Los tres NOP utilizados por V25.
  */

  digitalWrite(PGD_PIN, LOW);

  for (uint8_t i = 0; i < 3; i++)
  {
    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }

  Serial.println(F("NOP x3"));

  /*
     P9
  */

  digitalWrite(PGC_PIN, HIGH);

  Serial.println(F("P9 START"));

  delayMicroseconds(P9_US);

  digitalWrite(PGC_PIN, LOW);

  Serial.println(F("P9 END"));

  /*
     P10
  */

  delayMicroseconds(P10_US);

  Serial.println(F("P10 END"));

  /*
     Final
  */

  send16bit(0x0000);

  Serial.println(F("PROGRAMMING SEQUENCE TERMINADA"));
}

/* ============================================================
   PRUEBA 0x1234
   ============================================================ */

void test1234()
{
  Serial.println();
  Serial.println(F("########################################"));
  Serial.println(F(" V26B TEST 0x1234"));
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

  /*
     ERASE
  */

  eraseAll();

  uint8_t erased = readFlash(0x00, 0x00, 0x00);

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

  /*
     BUFFER
  */

  clearBuffer();

  writeBuffer[0] = 0x34;
  writeBuffer[1] = 0x12;

  Serial.println();
  Serial.println(F("BUFFER:"));

  for (uint8_t i = 0; i < 32; i++)
  {
    printHex8(writeBuffer[i]);
    Serial.print(' ');

    if (i == 15)
      Serial.println();
  }

  /*
     LOAD
  */

  loadWriteBuffer(0x000000);

  /*
     PROGRAM
  */

  programFlash();

  /*
     READ
  */

  uint8_t low = readFlash(0x00, 0x00, 0x00);
  uint8_t high = readFlash(0x00, 0x00, 0x01);

  uint16_t word =
    ((uint16_t)high << 8) |
    low;

  Serial.println();

  Serial.print(F("LOW  = 0x"));
  printHex8(low);
  Serial.println();

  Serial.print(F("HIGH = 0x"));
  printHex8(high);
  Serial.println();

  Serial.print(F("WORD = 0x"));
  printHex16(word);
  Serial.println();

  if (word == 0x1234)
  {
    Serial.println();
    Serial.println(F("********************************"));
    Serial.println(F(" TEST 1234 CORRECTO"));
    Serial.println(F("********************************"));
  }
  else
  {
    Serial.println();
    Serial.println(F("********************************"));
    Serial.println(F(" TEST 1234 FALLIDO"));
    Serial.println(F("********************************"));
  }

  exitLVP();
}

/* ============================================================
   INTEL HEX
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
   CONVERTIR BYTE HEX
   ============================================================ */

uint8_t hexByte(uint8_t pos)
{
  uint8_t h = hexValue(hexLine[pos]);
  uint8_t l = hexValue(hexLine[pos + 1]);

  if (h == 0xFF || l == 0xFF)
    return 0;

  return (h << 4) | l;
}

/* ============================================================
   RECIBIR UNA LINEA
   ============================================================ */

bool receiveHexLine()
{
  hexBaseAddress = 0;

  clearBuffer();

  rowActive = false;
  rowDirty = false;

  bool eof = false;
  bool error = false;

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("RECIBIENDO INTEL HEX"));
  Serial.println(F("================================"));

  /*
     Leer líneas hasta EOF.
  */

  while (!eof)
  {
    uint8_t pos = 0;

    /*
       Esperar ':'.
    */

    while (true)
    {
      while (!Serial.available());

      char c = Serial.read();

      if (c == ':')
        break;
    }

    hexLine[pos++] = ':';

    /*
       Leer hasta CR/LF.
    */

    while (true)
    {
      while (!Serial.available());

      char c = Serial.read();

      if (c == '\r' || c == '\n')
        break;

      if (pos < sizeof(hexLine) - 1)
      {
        hexLine[pos++] = c;
      }
      else
      {
        error = true;
      }
    }

    hexLen = pos;

    if (error)
      break;

    if (hexLen < 11)
    {
      Serial.println(F("ERROR: linea HEX demasiado corta"));
      error = true;
      break;
    }

    /*
       Procesar.
    */

    uint8_t type = hexByte(7);

    if (type == 0x01)
    {
      /*
         EOF.
      */

      if (!flushCurrentRow())
      {
        error = true;
        break;
      }

      eof = true;

      Serial.println();
      Serial.println(F("EOF INTEL HEX RECIBIDO"));
      break;
    }

    if (!processHexRecord())
    {
      error = true;
      break;
    }
  }

  if (error)
  {
    Serial.println();
    Serial.println(F("********************************"));
    Serial.println(F(" ERROR INTEL HEX"));
    Serial.println(F("********************************"));

    return false;
  }

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("INTEL HEX TERMINADO"));
  Serial.println(F("================================"));

  Serial.println(F("PROGRAMACION HEX FINALIZADA"));

  return true;
}

/* ============================================================
   PROCESAR REGISTRO HEX
   ============================================================ */

bool processHexRecord()
{
  /*
     Intel HEX:

     :LLAAAATTDD...CC

     LL = longitud
     AAAA = direccion
     TT = tipo
     DD = datos
     CC = checksum
  */

  uint8_t count = hexByte(1);

  uint16_t address =
    ((uint16_t)hexByte(3) << 8) |
    hexByte(5);

  uint8_t type = hexByte(7);

  uint8_t expectedLength =
    11 + (count * 2);

  if (hexLen < expectedLength)
  {
    Serial.println(F("ERROR: longitud HEX incorrecta"));
    return false;
  }

  /*
     Checksum.
  */

  uint8_t sum = 0;

  for (uint8_t i = 1; i < expectedLength; i += 2)
  {
    sum += hexByte(i);
  }

  if (sum != 0)
  {
    Serial.println(F("ERROR: CHECKSUM"));
    return false;
  }

  /*
     Tipo 00 = DATA
  */

  if (type == 0x00)
  {
    uint32_t absoluteAddress =
      hexBaseAddress + address;

    for (uint8_t i = 0; i < count; i++)
    {
      uint32_t a = absoluteAddress + i;

      /*
         PIC18F4550:
         Flash de programa:
         0x000000 - 0x007FFF
      */

      if (a > 0x007FFFUL)
      {
        /*
           No programamos todavía configuraciones,
           EEPROM o ID memory.
        */

        Serial.print(F("IGNORANDO ADDR 0x"));
        printHex32(a);
        Serial.println();

        continue;
      }

      /*
         Nueva fila de 32 bytes.
      */

      uint32_t newRow = a & 0xFFFFFFE0UL;

      if (!rowActive)
      {
        rowAddress = newRow;
        rowActive = true;
        rowDirty = false;
        clearBuffer();
      }
      else if (newRow != rowAddress)
      {
        if (!flushCurrentRow())
          return false;

        rowAddress = newRow;
        rowActive = true;
        rowDirty = false;
        clearBuffer();
      }

      uint8_t offset =
        (uint8_t)(a - rowAddress);

      writeBuffer[offset] =
        hexByte(9 + (i * 2));

      rowDirty = true;
    }

    return true;
  }

  /*
     Tipo 04 = Extended Linear Address.
  */

  if (type == 0x04)
  {
    if (count != 2)
    {
      Serial.println(F("ERROR: EXTENDED ADDRESS"));
      return false;
    }

    uint16_t upper =
      ((uint16_t)hexByte(9) << 8) |
      hexByte(11);

    hexBaseAddress =
      ((uint32_t)upper) << 16;

    Serial.print(F("EXTENDED ADDRESS = 0x"));
    printHex32(hexBaseAddress);
    Serial.println();

    return true;
  }

  /*
     Tipo 02 = Extended Segment Address.
     Lo aceptamos.
  */

  if (type == 0x02)
  {
    if (count != 2)
    {
      Serial.println(F("ERROR: SEGMENT ADDRESS"));
      return false;
    }

    uint16_t segment =
      ((uint16_t)hexByte(9) << 8) |
      hexByte(11);

    hexBaseAddress =
      ((uint32_t)segment) << 4;

    Serial.print(F("SEGMENT ADDRESS = 0x"));
    printHex32(hexBaseAddress);
    Serial.println();

    return true;
  }

  /*
     Otros tipos:
     03 START SEGMENT
     05 START LINEAR

     No son necesarios para programar el PIC.
  */

  return true;
}

/* ============================================================
   PROGRAMAR FILA ACTUAL
   ============================================================ */

bool flushCurrentRow()
{
  if (!rowActive || !rowDirty)
    return true;

  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("PROGRAMANDO ROW"));
  Serial.println(F("================================"));

  Serial.print(F("ROW = 0x"));
  printHex32(rowAddress);
  Serial.println();

  /*
     Mostrar primeros bytes.
  */

  Serial.print(F("DATA: "));

  for (uint8_t i = 0; i < 32; i++)
  {
    printHex8(writeBuffer[i]);
    Serial.print(' ');

    if (i == 15)
      Serial.println();
  }

  Serial.println();

  /*
     El PIC18F4550 programa rows de 32 bytes.
  */

  loadWriteBuffer(rowAddress);

  programFlash();

  /*
     Verificación de los 32 bytes.
  */

  Serial.println(F("VERIFY ROW"));

  bool ok = true;

  for (uint8_t i = 0; i < 32; i++)
  {
    uint32_t a = rowAddress + i;

    uint8_t value =
      readFlash(
        (a >> 16) & 0xFF,
        (a >> 8) & 0xFF,
        a & 0xFF
      );

    if (value != writeBuffer[i])
    {
      ok = false;

      Serial.print(F("ERROR ADDR 0x"));
      printHex32(a);

      Serial.print(F(" ESP=0x"));
      printHex8(writeBuffer[i]);

      Serial.print(F(" LEI=0x"));
      printHex8(value);

      Serial.println();
    }
  }

  if (ok)
  {
    Serial.println(F("ROW VERIFY: OK"));
  }
  else
  {
    Serial.println(F("ROW VERIFY: FALLA"));
  }

  rowActive = false;
  rowDirty = false;

  clearBuffer();

  return ok;
}

/* ============================================================
   SEND 4 BIT COMMAND
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
   SEND BYTE
   ============================================================ */

void sendbyte(uint8_t data)
{
  pinMode(PGD_PIN, OUTPUT);

  for (uint8_t i = 0; i < 8; i++)
  {
    if (data & (0x80 >> i))
      digitalWrite(PGD_PIN, HIGH);
    else
      digitalWrite(PGD_PIN, LOW);

    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }
}

/* ============================================================
   SEND 16 BIT
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
   SERIAL HEX
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

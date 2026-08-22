
/*
  ============================================================
   ARDUINO UNO -> PIC18F4550
   PROGRAMADOR LVP V27.1
   BASE V25/V26B FUNCIONAL

   FUNCIONES:
   D  = Device ID
   E  = Erase
   T  = Prueba 0x1234
   R 000000 = Leer Flash
   HEX = Recibir Intel HEX

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
   32 BYTES
   ============================================================ */

uint8_t writeBuffer[32];

/* ============================================================
   VARIABLES INTEL HEX
   ============================================================ */

uint8_t hexLine[70];
uint8_t hexLen;

uint32_t hexBaseAddress;

uint32_t rowAddress;
bool rowActive;
bool rowDirty;

bool hexFinished;

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

void commandDeviceID();
void commandErase();
void commandRead(uint32_t address);

void serialMenu();

uint8_t hexValue(char c);
uint8_t hexByte(uint8_t pos);

bool receiveHex();
bool processHexRecord();
bool flushCurrentRow();

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

  hexBaseAddress = 0;
  rowAddress = 0;
  rowActive = false;
  rowDirty = false;

  Serial.println();
  Serial.println(F("=========================================="));
  Serial.println(F(" ARDUINO UNO -> PIC18F4550"));
  Serial.println(F(" PROGRAMADOR LVP V27.1"));
  Serial.println(F(" INTEL HEX PROGRAMMER"));
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
  Serial.println(F("R 000000       = leer"));
  Serial.println(F("HEX            = cargar Intel HEX"));
  Serial.println();

  Serial.println(F("Arduino iniciado."));
  Serial.println(F("Serial = 115200"));
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

  char c = Serial.read();

  if (c == '\r' || c == '\n')
    return;

  if (c == 'T' || c == 't')
  {
    test1234();
    serialMenu();
    return;
  }

  if (c == 'D' || c == 'd')
  {
    commandDeviceID();
    serialMenu();
    return;
  }

  if (c == 'E' || c == 'e')
  {
    commandErase();
    serialMenu();
    return;
  }

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

  if (c == 'H' || c == 'h')
  {
    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F(" MODO INTEL HEX"));
    Serial.println(F("=========================================="));
    Serial.println(F("Envia ahora el contenido del build.hex"));
    Serial.println(F("El HEX debe comenzar con ':'."));
    Serial.println(F("EOF esperado: :00000001FF"));
    Serial.println();

    receiveHex();

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
  Serial.println(F("R 000000 = leer"));
  Serial.println(F("HEX = cargar build.hex"));
  Serial.println(F("------------------------------------------"));
}

/* ============================================================
   ENTRADA LVP
   RUTINA QUE YA FUNCIONA EN V25/V26B
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

  /*
    Secuencia MCHP
    4D 43 48 50
  */

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
    TBLRD*
  */

  send4bitcommand(0b1000);

  pinMode(PGD_PIN, INPUT);

  /*
    Dummy
  */

  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(PGC_PIN, HIGH);
    digitalWrite(PGC_PIN, LOW);
  }

  /*
    Read 8 bits
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

  /*
    TBLPTRU = 3C
  */

  send4bitcommand(0b0000);
  send16bit(0x0E3C);

  send4bitcommand(0b0000);
  send16bit(0x6EF8);

  /*
    TBLPTRH = 00
  */

  send4bitcommand(0b0000);
  send16bit(0x0E00);

  send4bitcommand(0b0000);
  send16bit(0x6EF7);

  /*
    TBLPTRL = 05
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
    Segunda etapa
  */

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
   RUTINA FUNCIONAL V25/V26B
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

  /*
    EEPGD = 1
    CFGS = 0
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
    15 pares
  */

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

  /*
    ULTIMO PAR

    Esta parte NO SE CAMBIA porque
    V25/V26B ya demostró que funciona.
  */

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

  /*
    NOP x3
  */

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
   TEST 1234
   ============================================================ */

void test1234()
{
  Serial.println();
  Serial.println(F("########################################"));
  Serial.println(F(" V27.1 TEST 0x1234"));
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

  uint8_t low =
    readFlash(0x00, 0x00, 0x00);

  uint8_t high =
    readFlash(0x00, 0x00, 0x01);

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
   HEX BYTE
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
   RECIBIR INTEL HEX
   ============================================================ */

bool receiveHex()
{
  hexBaseAddress = 0;

  rowActive = false;
  rowDirty = false;
  hexFinished = false;

  Serial.println(F("ESPERANDO INTEL HEX..."));

  while (!hexFinished)
  {
    /*
      Buscar ':'
    */

    while (true)
    {
      while (!Serial.available());

      char c = Serial.read();

      if (c == ':')
        break;
    }

    hexLen = 0;
    hexLine[hexLen++] = ':';

    /*
      Leer resto de línea
    */

    while (true)
    {
      while (!Serial.available());

      char c = Serial.read();

      if (c == '\r' || c == '\n')
        break;

      if (hexLen < sizeof(hexLine) - 1)
      {
        hexLine[hexLen++] = c;
      }
      else
      {
        Serial.println(F("ERROR: LINEA DEMASIADO LARGA"));

        while (Serial.available())
          Serial.read();

        return false;
      }
    }

    /*
      Necesitamos como mínimo:
      :
      LL
      AAAA
      TT
      CC

      11 caracteres.
    */

    if (hexLen < 11)
    {
      Serial.println(F("ERROR: LINEA HEX CORTA"));
      return false;
    }

    uint8_t type = hexByte(7);

    /*
      EOF
    */

    if (type == 0x01)
    {
      if (!flushCurrentRow())
        return false;

      hexFinished = true;

      Serial.println();
      Serial.println(F("EOF INTEL HEX RECIBIDO"));
      Serial.println(F("INTEL HEX TERMINADO"));

      return true;
    }

    /*
      Procesar registro
    */

    if (!processHexRecord())
      return false;
  }

  return true;
}

/* ============================================================
   PROCESAR REGISTRO INTEL HEX
   ============================================================ */

bool processHexRecord()
{
  /*
    :LLAAAATTDD...CC
  */

  uint8_t count = hexByte(1);

  uint16_t address =
    ((uint16_t)hexByte(3) << 8) |
    hexByte(5);

  uint8_t type = hexByte(7);

  uint8_t expected =
    11 + (count * 2);

  if (hexLen < expected)
  {
    Serial.println(F("ERROR: LONGITUD HEX"));
    return false;
  }

  /*
    CHECKSUM
  */

  uint8_t sum = 0;

  for (uint8_t i = 1; i < expected; i += 2)
    sum += hexByte(i);

  if (sum != 0)
  {
    Serial.println(F("ERROR: CHECKSUM"));
    return false;
  }

  /*
    DATA
  */

  if (type == 0x00)
  {
    uint32_t absolute =
      hexBaseAddress + address;

    for (uint8_t i = 0; i < count; i++)
    {
      uint32_t a =
        absolute + i;

      /*
        PIC18F4550:
        Flash programa hasta 0x7FFF
      */

      if (a > 0x007FFFUL)
      {
        Serial.print(F("IGNORANDO 0x"));
        printHex32(a);
        Serial.println();

        continue;
      }

      /*
        Row de 32 bytes
      */

      uint32_t newRow =
        a & 0xFFFFFFE0UL;

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
    EXTENDED LINEAR ADDRESS
    Tipo 04
  */

  if (type == 0x04)
  {
    if (count != 2)
    {
      Serial.println(F("ERROR: TIPO 04"));
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
    EXTENDED SEGMENT
    Tipo 02
  */

  if (type == 0x02)
  {
    if (count != 2)
    {
      Serial.println(F("ERROR: TIPO 02"));
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
    Tipos 03 y 05:
    Start Segment / Start Linear.
    Se ignoran.
  */

  return true;
}

/* ============================================================
   PROGRAMAR ROW
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
    ENTRAR LVP PARA ESTA OPERACION
  */

  enterLVP();

  uint16_t id = readDeviceID();

  if (id != 0x1207)
  {
    Serial.println(F("ERROR DEVICE ID"));
    exitLVP();

    rowActive = false;
    rowDirty = false;

    return false;
  }

  /*
    PROGRAMAR
  */

  loadWriteBuffer(rowAddress);
  programFlash();

  /*
    VERIFICACION
  */

  Serial.println();
  Serial.println(F("VERIFY ROW"));

  bool ok = true;

  for (uint8_t i = 0; i < 32; i++)
  {
    uint32_t a =
      rowAddress + i;

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
    Serial.println(F("ROW VERIFY: OK"));
  else
    Serial.println(F("ROW VERIFY: FALLA"));

  exitLVP();

  rowActive = false;
  rowDirty = false;

  clearBuffer();

  return ok;
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
   HEX PRINT
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


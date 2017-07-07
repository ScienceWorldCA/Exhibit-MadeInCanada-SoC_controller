// Microcontroller code for the Sounds of Canada exhibit for
// Science World.
// Jeff Cowan
//
// Hardware: Teensy LC
// USB Type: "Serial + Keyboard + Mouse + Joystick"
// Keyboard Layout: "US English"
//
// Version 1.0 Feb 16, 2017
// Version 1.1 Mar 18, 2017 - Test build for NGX
// Version 1.2 Apr 16, 2017 - Added analog deadband and reduced transmission
//                            speed to prevent buffer overflow in the NUC.

const byte ANALOG_PINS[] = {19, 18, 17, 16, 15, 14};  // 10k potentiometers
const byte MUX_PINS[] = {6, 7, 8};  // 74LS153 multiplexer
const byte READ_PINS[] = {12, 11, 10, 9};   // reed switches
const byte LED_PINS[] = {0, 1, 2, 3, 4, 5}; // ULN2008 transistor array driving 12VDC blue LED monoblock indicators

const byte MUX_MASK_1 = 0b00000001;
const byte MUX_MASK_2 = 0b00000010;
const byte MUX_MASK_3 = 0b00000100;

typedef struct {
  byte objectID;      // 0x00 to 0x0F (only lower 4 bits used)
  byte volumeSetting; // 0x00 to 0xFF
} Channel;

Channel newData[6];
Channel oldData[6];
boolean sendFlag[6];
byte sendIndex = 0;

//=========================================================
void setup() {
  analogReference(DEFAULT); // use 3.3V power as analog reference voltage (Teensy LC does not have internal VREF)
  pinMode(MUX_PINS[0], OUTPUT);
  pinMode(MUX_PINS[1], OUTPUT);
  pinMode(MUX_PINS[2], OUTPUT);
  pinMode(READ_PINS[0], INPUT);
  pinMode(READ_PINS[1], INPUT);
  pinMode(READ_PINS[2], INPUT);
  pinMode(READ_PINS[3], INPUT);
  pinMode(LED_PINS[0], OUTPUT);
  pinMode(LED_PINS[1], OUTPUT);
  pinMode(LED_PINS[2], OUTPUT);
  pinMode(LED_PINS[3], OUTPUT);
  pinMode(LED_PINS[4], OUTPUT);
  pinMode(LED_PINS[5], OUTPUT);
  delay(5000); // Delay 5 seconds to allow the computer to enumerate Teensy LC as a new USB keyboard.
}

//=========================================================
void loop() {

  // Transmits the next frame in the transmission queue.
  for (byte j = 0; j < 6; j++) {
    byte n = (j + sendIndex) % 6;
    if (sendFlag[n]) {
      transmit(n + 1, newData[n]);
      sendFlag[n] = false;
      sendIndex++;
    }
  }
  delay(50);

  // Reads all input data and compares it to previous state.
  // Any changed data will be added to the transmit queue.
  for (byte i = 0; i < 6; i++) {
    scan(i);
    if ((newData[i].objectID != oldData[i].objectID) ||
        (newData[i].volumeSetting - oldData[i].volumeSetting > 1) ||
        (newData[i].volumeSetting - oldData[i].volumeSetting < -1)) {
      sendFlag[i] = true;
      oldData[i].objectID = newData[i].objectID;
      oldData[i].volumeSetting = newData[i].volumeSetting;
    }
  }

  // Sets the LED outputs to indicate active slots.
  for (byte i = 0; i < 6; i++)
  {
    if (newData[i].objectID == 0)
      digitalWrite(LED_PINS[i], LOW);
    else
      digitalWrite(LED_PINS[i], HIGH);
  }
}

//---------------------------------------------------------
void scan(byte number)
{
  digitalWrite(MUX_PINS[0], number & MUX_MASK_1);
  digitalWrite(MUX_PINS[1], number & MUX_MASK_2);
  digitalWrite(MUX_PINS[2], number & MUX_MASK_3);

  // read the state of the four reed switches and store as a four-bit number
  // objects:
  // b0000  0
  // b0001  1
  // b0010  2
  // b0011  3
  // b0100  4
  // b0101  5
  // b0110  6
  // b0111  7
  // b1000  8
  // b1001  9
  // b1010  10
  // b1011  11
  // b1100  12
  // b1101  13
  // b1110  14
  // b1111  15
  newData[number].objectID = !digitalRead(READ_PINS[0]) +
                             (!digitalRead(READ_PINS[1]) << 1) +
                             (!digitalRead(READ_PINS[2]) << 2) +
                             (!digitalRead(READ_PINS[3]) << 3);

  newData[number].volumeSetting = analogRead(ANALOG_PINS[number]) / 4;
  if (newData[number].volumeSetting <= 0x08)
  {
    newData[number].volumeSetting = 0;  // clamp low values to zero
  }
}

//--------------------------------------------------------
void transmit(byte number, Channel data)  // sends 6 key presses (4 data, plus two for Carriage Return and New Line) 
{
  Keyboard.print(number, HEX);  // first character (1 to 6)
  Keyboard.print(data.objectID, HEX); // second character object ID (0 to F)
  if (data.volumeSetting <= 0x0F)
  {
    Keyboard.print("0"); // print leading zero (third character) if required
  }
  Keyboard.print(data.volumeSetting, HEX); // third and fourth characters volume (0 to FF)
  Keyboard.print("\r\n"); // Windows uses \r\n as a new line
}

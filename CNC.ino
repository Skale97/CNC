#define X_positive PORTC |= 32; PORTC &= ~8;
#define X_negative PORTC |= 8; PORTC &= ~32;
#define X_stop PORTC &= ~40;
#define Y_positive PORTC |= 16; PORTC &= ~4;
#define Y_negative PORTC |= 4; PORTC &= ~16;
#define Y_stop PORTC &= ~20;
#define Z_positive PORTD |= 128; PORTD &= ~16;
#define Z_negative PORTD |= 16; PORTD &= ~128;
#define Z_stop PORTD &= ~144;

const char space[50] = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
volatile byte *pwmx;
volatile byte *pwmy;
volatile byte *pwmz;
char lut[4][4] = {{0, 1, -1, 0}, { -1, 0, 0, 1}, {1, 0, 0, -1}, {0, -1, 1, 0}};
bool exitMenu = false;
volatile int X_position = 0;
volatile int Y_position = 0;
volatile int Z_position = 0;
volatile int X_destination = 0;
volatile int Y_destination = 0;
volatile int Z_destination = 0;
volatile int X_error = 0;
volatile int Y_error = 0;
volatile int Z_error = 0;

int X_last_error = 0;
int Y_last_error = 0;
int Z_last_error = 0;

volatile byte X_signal = 0;
volatile byte Y_signal = 0;
volatile byte Z_signal = 0;
volatile byte X_last_signal = 0;
volatile byte Y_last_signal = 0;
volatile byte Z_last_signal = 0;

float X_out = 0;
float X_Kp = 0.08;
float X_Ki = 0.005;
float X_Kd = 0.005;
float X_Integrator_state = 0;

float Y_out = 0;
float Y_Kp = 0.1;
float Y_Ki = 0.001;
float Y_Kd = 0.005;
float Y_Integrator_state = 0;

float Z_out = 0;
float Z_Kp = 0.01;
float Z_Ki = 0.0001;
float Z_Kd = 0.0005;
float Z_Integrator_state = 0;

bool AutoMove = false;
bool ConstData = false;
bool MoveOnData = false;

volatile byte k = 0;
byte cs = 0;

String strbuff = ".";

void setup() {
  Serial.begin(57600);
  DDRB = B00001000;
  DDRC = B00111100;
  DDRD = B11110010;

  PCICR = B00000111;
  PCMSK2 = B00001100;
  PCMSK1 = B00000011;
  PCMSK0 = B00000011;

  TCCR0A = B10100001;
  TCCR0B = B00000011;
  pwmz = &OCR0A;
  pwmy = &OCR0B;
  TIMSK2 = B00000000;

  TCCR2A = B10100001;
  TCCR2B = B00000101;
  pwmx = &OCR2A;
  TIMSK2 = B00000000;

  TCCR1A = B00000000;
  TCCR1B = B00001011;//64psc
  TCCR1C = B00000000;
  OCR1A = 338;//800Hz
  TIMSK1 = B00000010;

  interrupts();

  ConstData = true;

  Serial.print(k);
  Serial.print(F("Write any char to enter menu\n"));
}

void loop() {
  if (Serial.available() > 0) {
    Serial.read();
    if (MoveOnData) {
      Serial.print("X");
      while (Serial.available() <= 0);
      if (Serial.peek() == 255) MoveOnData = false;
      else {
        X_destination = Serial.read() * 100;
        while (Serial.available() <= 0);
        X_destination += Serial.read();

        while (Serial.available() <= 0);
        Y_destination = Serial.read() * 100;
        while (Serial.available() <= 0);
        Y_destination += Serial.read();

        while (Serial.available() <= 0);
        Z_destination = Serial.read() * 100;
        while (Serial.available() <= 0);
        Z_destination += Serial.read();
        AutoMove = true;
        Serial.print("DONE");
      }
    }
    else {
      Serial.print(space);
      *pwmx = 0;
      *pwmy = 0;
      *pwmz = 0;
      menu();
    }
  }
  if ((cs >= 40) && ConstData) {
    cs = 0;
    strbuff = " ";
    strbuff += space;
    strbuff += "xc: ";
    strbuff += X_position;
    strbuff += "   yc: ";
    strbuff += Y_position;
    strbuff += "   zc: ";
    strbuff += Z_position;
    strbuff += "\nxd: ";
    strbuff += X_destination;
    strbuff += "   yd: ";
    strbuff += Y_destination;
    strbuff += "   zd: ";
    strbuff += Z_destination;
    strbuff += "\nxp: ";
    strbuff += *pwmx;
    strbuff += "   yp: ";
    strbuff += *pwmy;
    strbuff += "   zp: ";
    strbuff += *pwmz;
    Serial.print(strbuff);
  }
  if (k >= 2) {
    k = 0;
    cs++;
    if (AutoMove) {
      if (X_error == Y_error == Z_error == X_last_error == Y_last_error == Z_last_error == 0) {
        Serial.print("1");
        AutoMove = false;
      }

      X_error = X_destination - X_position;
      Y_error = Y_destination - Y_position;
      Z_error = Z_destination - Z_position;

      if (X_error == 0) X_Integrator_state = 0;
      if (Y_error == 0) Y_Integrator_state = 0;
      if (Z_error == 0) Z_Integrator_state = 0;

      X_out = (X_Kp * X_error + X_Ki * X_Integrator_state + X_Kd * (X_last_error - X_error));
      Y_out = (Y_Kp * Y_error + Y_Ki * Y_Integrator_state + Y_Kd * (Y_last_error - Y_error));
      Z_out = (Z_Kp * Z_error + Z_Ki * Z_Integrator_state + Z_Kd * (Z_last_error - Z_error));


      if (X_Integrator_state < 250) X_Integrator_state += X_error;
      if (Y_Integrator_state < 250) Y_Integrator_state += Y_error;
      if (Z_Integrator_state < 250) Z_Integrator_state += Z_error;

      X_last_error = X_error;
      Y_last_error = Y_error;
      Z_last_error = Z_error;

      if (X_out > 0) {
        if (X_out > 150) *pwmx = 245;
        else *pwmx = X_out + 95;
        X_positive
      }
      else if (X_out < 0) {
        if (X_out < -150) *pwmx = 245;
        else *pwmx = -X_out + 95;
        X_negative
      }
      else {
        *pwmx = 0;
        X_stop
      }

      if (Y_out > 0) {
        if (Y_out > 199) *pwmy = 255;
        else *pwmy = Y_out + 56;
        Y_positive
      }
      else if (Y_out < 0) {
        if (Y_out < -199) *pwmy = 255;
        else *pwmy = -Y_out + 56;
        Y_negative
      }
      else {
        *pwmy = 0;
        Y_stop
      }

      if (Z_out > 0) {
        if (Z_out > 205) *pwmz = 255;
        else *pwmz = Z_out + 50;
        Z_positive
      }
      else if (Z_out < 0) {
        if (Z_out < -205) *pwmz = 255;
        else *pwmz = -Z_out + 50;
        Z_negative
      }
      else {
        *pwmz = 0;
        Z_stop
      }
    }
  }
}


void menu() {
  exitMenu = false;
  byte buff = 0;
  while (!exitMenu) {
    strbuff = "\n";
    strbuff += F("0  - Curr x: ");
    strbuff += X_position;
    strbuff += F("\n1  - Curr y: ");
    strbuff += Y_position;
    strbuff += F("\n2  - Curr z: ");
    strbuff += Z_position;
    strbuff += F("\n3  - Dest x: ");
    strbuff += X_destination;
    strbuff += F("\n4  - Dest y: ");
    strbuff += Y_destination;
    strbuff += F("\n5  - Dest z: ");
    strbuff += Z_destination;
    strbuff += F("\n6  - PWM x: ");
    strbuff += *pwmx;
    strbuff += F("\n7  - PWM y: ");
    strbuff += *pwmy;
    strbuff += F("\n8  - PWM z: ");
    strbuff += *pwmz;
    strbuff += F("\n9  - Dir x: ");
    if (PINC & 32) strbuff += F("P");
    else if (PINC & 8) strbuff += F("N");
    else strbuff += F("S");
    strbuff += F("\n10 - Dir y: ");
    if (PINC & 16) strbuff += F("P");
    else if (PINC & 4) strbuff += F("N");
    else strbuff += F("S");
    strbuff += F("\n11 - Dir z: ");
    if (PIND & 128) strbuff += F("P");
    else if (PIND & 16) strbuff += F("N");
    else strbuff += F("S");
    strbuff += F("\n12 - Auto move: ");
    if (AutoMove) strbuff += F("I");
    else strbuff += F("O");
    strbuff += F("\n13 - Const data: ");
    if (ConstData) strbuff += F("I");
    else strbuff += F("O");
    strbuff += F("\n14 - Move on data: ");
    if (MoveOnData) strbuff += F("I");
    else strbuff += F("O");
    strbuff += F("\n15 - Go to zero");
    strbuff += F("\n16 - Dismount z");
    strbuff += F("\n17 - Refresh");
    strbuff += F("\n18 - EXIT\n");
    Serial.print(strbuff);
    while (Serial.available() <= 0);
    Serial.print(space);
    buff = Serial.read();
    if (buff == 0) {
      X_position = serialInt(X_position);
    }
    else if (buff == 1) {
      Y_position = serialInt(Y_position);
    }
    else if (buff == 2) {
      Z_position = serialInt(Z_position);
    }
    else if (buff == 3) {
      X_destination = serialInt(X_destination);
    }
    else if (buff == 4) {
      Y_destination = serialInt(Y_destination);
    }
    else if (buff == 5) {
      Z_destination = serialInt(Z_destination);
    }
    else if (buff == 6) {
      Serial.print("1 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 1) *pwmx = buff;
    }
    else if (buff == 7) {
      Serial.print("1 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 1) *pwmy = buff;
    }
    else if (buff == 8) {
      Serial.print("1 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 1) *pwmz = buff;
    }
    else if (buff == 9) {
      Serial.print("0 - S, 1 - P, 2 - N, 3 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 3) {
        if (buff == 0) {
          X_stop
        }
        else if (buff == 1) {
          X_positive
        }
        else if (buff == 2) {
          X_negative
        }
      }
    }
    else if (buff == 10) {
      Serial.print("0 - S, 1 - P, 2 - N, 3 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 3) {
        if (buff == 0) {
          Y_stop
        }
        else if (buff == 1) {
          Y_positive
        }
        else if (buff == 2) {
          Y_negative
        }
      }
    }
    else if (buff == 11) {
      Serial.print("0 - S, 1 - P, 2 - N, 3 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 3) {
        if (buff == 0) {
          Z_stop
        }
        else if (buff == 1) {
          Z_positive
        }
        else if (buff == 2) {
          Z_negative
        }
      }
    }
    else if (buff == 12) {
      AutoMove = OnOff(AutoMove);
    }
    else if (buff == 13) {
      ConstData = OnOff(ConstData);
    }
    else if (buff == 14) {
      MoveOnData = OnOff(MoveOnData);
    }
    else if (buff == 15) {

    }
    else if (buff == 16) {

    }
    else if (buff == 17) {
      //Refresh
    }
    else if (buff == 18) {
      exitMenu = true;
      Serial.print("Write any char to enter menu");
    }
  }
}

int serialInt(int last) {
  int integer = last;
  Serial.print("Write upper digits - 255 to cancel\n");
  while (Serial.available() <= 0);
  byte buf = Serial.read();
  if (buf != 255) {
    integer = buf * 100;
    Serial.print(space);
    Serial.print(space);
    Serial.print("Write lower 2 digits\n");
    while (Serial.available() <= 0);
    integer += Serial.read();
  }
  Serial.print(space);
  Serial.print(space);
  return integer;
}

bool OnOff(bool a) {
  bool b = a;
  Serial.print("0 - O, 1 - I, 2 - canc\n");
  while (Serial.available() <= 0);
  byte buff = Serial.read();
  if (buff != 2) {
    if (buff == 0) b = false;
    else if (buff == 1) b = true;
  }
  return b;
}

ISR(TIMER1_COMPA_vect) {
  k++;
}

ISR(PCINT0_vect) {
  Z_signal = PINB & B00000011;
  Z_position += lut[Z_signal][Z_last_signal];
  Z_last_signal = Z_signal;
}

ISR(PCINT1_vect) {
  Y_signal = PINC & B00000011;
  Y_position += lut[Y_last_signal][Y_signal];
  Y_last_signal = Y_signal;
}

ISR(PCINT2_vect) {
  X_signal = (PIND >> 2) & B00000011;
  X_position += lut[X_signal][X_last_signal];
  X_last_signal = X_signal;
}



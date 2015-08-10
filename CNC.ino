#define xp PORTC |= 4; PORTC &= ~8;
#define xn PORTC |= 8; PORTC &= ~4;
#define xst PORTC &= ~12;
#define yp PORTC |= 16; PORTC &= ~32;
#define yn PORTC |= 32; PORTC &= ~16;
#define yst PORTC &= ~48;
#define zp PORTD |= 16; PORTD &= ~128;
#define zn PORTD |= 128; PORTD &= ~16;
#define zst PORTD &= ~144;

char space[50] = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
volatile byte *pwmx;
volatile byte *pwmy;
volatile byte *pwmz;
char lut[4][4] = {{0, 1, -1, 0}, { -1, 0, 0, 1}, {1, 0, 0, -1}, {0, -1, 1, 0}};
bool exitMenu = false;
volatile int xpos = 0;
volatile int ypos = 0;
volatile int zpos = 0;
volatile int xdes = 0;
volatile int ydes = 0;
volatile int zdes = 0;

volatile byte xs = 0;
volatile byte ys = 0;
volatile byte zs = 0;
volatile byte xls = 0;
volatile byte yls = 0;
volatile byte zls = 0;

bool AutoMove = false;
bool ConstData = false;

volatile byte k = 0;
byte cs = 0;

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
  TCCR0B = B00000010;
  pwmx = &OCR0A;
  pwmy = &OCR0B;
  TIMSK2 = B00000000;

  TCCR2A = B10100001;
  TCCR2B = B00000010;
  pwmz = &OCR2A;
  TIMSK2 = B00000000;

  TCCR1A = B00000000;
  TCCR1B = B00001011;
  TCCR1C = B00000000;
  OCR1A = 1251;
  TIMSK1 = B00000010;

  interrupts();

  *pwmx = 50;
  *pwmy = 125;
  *pwmz = 200;
  ConstData = true;

  Serial.print(k);
  Serial.print("Write any char to enter menu\n");
}

void loop() {
  if (Serial.available() > 0) {
    Serial.print(k);
    Serial.print(space);
    Serial.read();
    *pwmx = 0;
    *pwmy = 0;
    *pwmz = 0;
    menu();
  }
  if ((cs >= 8) && ConstData) {
    cs = 0;
    Serial.print(space);
    Serial.print("xc: ");
    Serial.print(xpos);
    Serial.print("   yc: ");
    Serial.print(ypos);
    Serial.print("   zc: ");
    Serial.print(zpos);
    Serial.print("\n");
    Serial.print("xd: ");
    Serial.print(xdes);
    Serial.print("   yd: ");
    Serial.print(ydes);
    Serial.print("   zd: ");
    Serial.print(zdes);
    Serial.print("\n");
    Serial.print("xp: ");
    Serial.print(*pwmx);
    Serial.print("   yp: ");
    Serial.print(*pwmx);
    Serial.print("   zp: ");
    Serial.print(*pwmx);
  }
  if (k >= 2) {
    k = 0;
    cs++;
    if (AutoMove) {
      Serial.print("a");
    }
  }
}

void menu() {
  exitMenu = false;
  byte buff = 0;
  while (!exitMenu) {
    Serial.print("0  - Curr x: ");
    Serial.print(xpos);
    Serial.print("\n1  - Curr y: ");
    Serial.print(ypos);
    Serial.print("\n2  - Curr z: ");
    Serial.print(zpos);
    Serial.print("\n3  - Dest x: ");
    Serial.print(xdes);
    Serial.print("\n4  - Dest y: ");
    Serial.print(ydes);
    Serial.print("\n5  - Dest z: ");
    Serial.print(zdes);
    Serial.print("\n6  - PWM x: ");
    buff = *pwmx;
    Serial.print(buff);
    Serial.print("\n7  - PWM y: ");
    buff = *pwmy;
    Serial.print(buff);
    Serial.print("\n8  - PWM z: ");
    buff = *pwmz;
    Serial.print(buff);
    Serial.print("\n9  - Dir x: ");
    if (PINC & 4) Serial.print("P");
    else if (PINC & 8) Serial.print("N");
    else Serial.print("S");
    Serial.print("\n10 - Dir y: ");
    if (PINC & 16) Serial.print("P");
    else if (PINC & 32) Serial.print("N");
    else Serial.print("S");
    Serial.print("\n11 - Dir z: ");
    if (PIND & 16) Serial.print("P");
    else if (PIND & 128) Serial.print("N");
    else Serial.print("S");
    Serial.print("\n12 - Auto move: ");
    if (AutoMove) Serial.print("I");
    else Serial.print("O");
    Serial.print("\n13 - Const data: ");
    if (ConstData) Serial.print("I");
    else Serial.print("O");
    Serial.print("\n14 - Go to zero");
    Serial.print("\n15 - Dismount z");
    Serial.print("\n16 - Refresh");
    Serial.print("\n17 - EXIT\n");

    while (Serial.available() <= 0);
    Serial.print(space);
    buff = Serial.read();
    if (buff == 0) {
      xpos = serialInt(xpos);
    }
    else if (buff == 1) {
      ypos = serialInt(ypos);
    }
    else if (buff == 2) {
      zpos = serialInt(zpos);
    }
    else if (buff == 3) {
      xdes = serialInt(xdes);
    }
    else if (buff == 4) {
      ydes = serialInt(ydes);
    }
    else if (buff == 5) {
      zdes = serialInt(zdes);
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
          xst
        }
        else if (buff == 1) {
          xp
        }
        else if (buff == 2) {
          xn
        }
      }
    }
    else if (buff == 10) {
      Serial.print("0 - S, 1 - P, 2 - N, 3 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 3) {
        if (buff == 0) {
          yst
        }
        else if (buff == 1) {
          yp
        }
        else if (buff == 2) {
          yn
        }
      }
    }
    else if (buff == 11) {
      Serial.print("0 - S, 1 - P, 2 - N, 3 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 3) {
        if (buff == 0) {
          zst
        }
        else if (buff == 1) {
          zp
        }
        else if (buff == 2) {
          zn
        }
      }
    }
    else if (buff == 12) {
      Serial.print("0 - O, 1 - I, 2 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 2) {
        if (buff == 0) AutoMove = false;
        else if (buff == 1) AutoMove = true;
      }
    }
    else if (buff == 13) {
      Serial.print("0 - O, 1 - I, 2 - canc\n");
      while (Serial.available() <= 0);
      buff = Serial.read();
      if (buff != 2) {
        if (buff == 0) ConstData = false;
        else if (buff == 1) ConstData = true;
      }
    }
    else if (buff == 14) {

    }
    else if (buff == 15) {

    }
    else if (buff == 17) {
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
  return integer;
}

ISR(TIMER1_COMPA_vect) {
  k++;
}

ISR(PCINT0_vect) {
  zs = PINB & B00000011;
  zpos += lut[zs][zls];
  zls = zs;
}

ISR(PCINT1_vect) {
  ys = PINC & B00000011;
  ypos += lut[ys][yls];
  yls = ys;
}

ISR(PCINT2_vect) {
  xs = (PIND >> 2) & B00000011;
  xpos += lut[xs][xls];
  xls = xs;
}



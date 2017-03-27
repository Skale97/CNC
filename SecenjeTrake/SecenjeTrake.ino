#define X_positive PORTC |= 32; PORTC &= ~8;
#define X_negative PORTC |= 8; PORTC &= ~32;
#define X_stop PORTC &= ~40;
#define Y_positive PORTC |= 16; PORTC &= ~4;
#define Y_negative PORTC |= 4; PORTC &= ~16;
#define Y_stop PORTC &= ~20;

const char space[50] = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

volatile byte *pwmx;
volatile byte *pwmy;

char lut[4][4] = {{0, 1, -1, 0}, { -1, 0, 0, 1}, {1, 0, 0, -1}, {0, -1, 1, 0}};

volatile int X_position = 0;
volatile long Y_position = 0;
volatile int X_destination = 0;
volatile long Y_destination = 0;
volatile int X_error = 0;
volatile long Y_error = 0;
volatile int memX = 0;
volatile long memY = 0;
volatile int X_last_error = 0;
volatile long Y_last_error = 0;

volatile byte X_signal = 0;
volatile byte Y_signal = 0;
volatile byte X_last_signal = 0;
volatile byte Y_last_signal = 0;

float X_out = 0;
float X_Kp = 0.75;
float X_Ki = 0.005;
float X_Kd = 3.25;
float X_Integrator_state = 0;

float Y_out = 0;
float Y_Kp = 0.5;
float Y_Ki = 0.01;
float Y_Kd = 20;
float Y_Integrator_state = 0;

bool AutoMove = false;
bool working = false;

volatile byte k = 0;
volatile byte j = 0;
volatile byte state = 0;

String strbuff = ".";

int brojTraka = 0;
int trenutniBrojTraka = 0;
long duzinaTrake = 0l;
byte brzinaNoza = 0;
byte brzinaTrake = 0;

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
  OCR0A = 0;
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

  brzinaTrake = 200;
  brzinaNoza = 220;
  Serial.print(F("Write any char to enter menu\n"));
}

void loop() {
  if (Serial.available() > 0) {
    if (!working) {
      Serial.read();
      Serial.print(space);
      *pwmx = 0;
      *pwmy = 0;
      Serial.print(F("Unesite broj traka (default=0):\n"));
      brojTraka = serialInt(brojTraka);
      Serial.print(F("Unesite duzinu trake (default=0):\n"));
      while(Serial.available() < 1){}
      duzinaTrake = 1000l*Serial.read();
      trenutniBrojTraka = 0;
      state = 0;
      working = true;
      AutoMove = true;
    } else {
      Serial.read();
      AutoMove = false;
      *pwmx = 0;
      *pwmy = 0;
      Serial.print("Ako zelite da prekinete unesite 255\n");
      while (Serial.available() <= 0) {}
      if (Serial.read() == 255) {
        working = false;
        brojTraka = 0;
        duzinaTrake = 0;
        brzinaNoza = 0;
        brzinaTrake = 0;
        X_position = X_destination = X_error = 0;
        Y_position = Y_destination = Y_error = 0;
        trenutniBrojTraka = 0;
        state = 0;
      }
      AutoMove = true;
    }
  }
  if (working && j >= 100) {
    if (state == 0) {
      X_destination = 1800;
      state = 1;
    }
    if (state == 1 && memX <= 1810 && memX > 1000 && X_position < 1810 && X_position >= 1800) {
      X_destination = 0;
      state = 2;
    }
    if (state == 2 && memX <= 0 && memX > -10 && X_position <= 0 && X_position > -10) {
      X_destination = X_position;
      X_error = 0;
      Y_destination = duzinaTrake;
      state = 3;
      trenutniBrojTraka++;
      Serial.print(trenutniBrojTraka);
      Serial.print("/");
      Serial.print(brojTraka);
      Serial.print("\n");
      if(trenutniBrojTraka >= brojTraka){
        Serial.print("Gotovo\n");
        working = false;
        brojTraka = 0;
        duzinaTrake = 0;
        brzinaNoza = 0;
        brzinaTrake = 0;
        X_position = X_destination = X_error = 0;
        Y_position = Y_destination = Y_error = 0;
        trenutniBrojTraka = 0;
        state = 0;
      }
    }
    if (state == 3 && memY < duzinaTrake + 5 && memY > duzinaTrake - 5 && Y_position < duzinaTrake + 5 && Y_position > duzinaTrake - 5) {
      Y_position = Y_destination = 0;
      Y_error = 0;
      state = 0;
    }

    memX = X_position;
    memY = Y_position;
  }
  if (k >= 2) {
    k = 0;
    j++;

    X_last_error = X_error;
    Y_last_error = Y_error;

    X_error = X_destination - X_position;
    Y_error = Y_destination - Y_position;

    if (X_error == 0 || sgn(X_error) != sgn(X_last_error)) X_Integrator_state = 0;
    if (Y_error == 0 || sgn(Y_error) != sgn(Y_last_error)) Y_Integrator_state = 0;

    X_out = (X_Kp * X_error + X_Ki * X_Integrator_state - X_Kd * (X_last_error - X_error));
    Y_out = (Y_Kp * Y_error + Y_Ki * Y_Integrator_state - Y_Kd * (Y_last_error - Y_error));

    if (X_Integrator_state < 250 && X_Integrator_state > -250) X_Integrator_state += X_error;
    if (Y_Integrator_state < 250 && Y_Integrator_state > -250) Y_Integrator_state += Y_error;

    if (X_out > 0) {
      if (X_out > brzinaNoza - 105) *pwmx = brzinaNoza;
      else *pwmx = X_out + 105;
      X_positive
    }
    else if (X_out < 0) {
      if (X_out < -brzinaNoza + 105) *pwmx = brzinaNoza;
      else *pwmx = -X_out + 105;
      X_negative
    }
    else {
      *pwmx = 0;
      X_stop
    }

    if (Y_out > 0) {
      if (Y_out > brzinaTrake - 56) *pwmy = brzinaTrake;
      else *pwmy = Y_out + 56;
      Y_positive
    }
    else if (Y_out < 0) {
      if (Y_out < -brzinaTrake + 56) *pwmy = brzinaTrake;
      else *pwmy = -Y_out + 56;
      Y_negative
    }
    else {
      *pwmy = 0;
      Y_stop
    }
  }
}


int serialInt(int last) {
  int integer = last;
  Serial.print("Write upper digits - 255 to leave default\n");
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

static inline byte sgn(int val) {
  if (val < 0) return -1;
  if (val == 0) return 0;
  return 1;
}

ISR(TIMER1_COMPA_vect) {
  k++;
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



#define c 261
#define d 294
#define e 329
#define f 349
#define g 391
#define gS 415
#define a 440
#define aS 455
#define b 466
#define cH 523
#define cSH 554
#define dH 587
#define dSH 622
#define eH 659
#define fH 698
#define fSH 740
#define gH 784
#define gSH 830
#define aH 880

int channel = 0;
int frequency = 4000;
int resolution = 8;

void setup() {
  ledcSetup(channel, frequency, resolution);
  ledcAttachPin(2, channel);
}

void loop() {
  // put your main code here, to run repeatedly:
  imperial_march();
}

void beep(int channel, int frequencyHZ, long durance) {

  long delayAmount = (long)(1000000 / frequencyHZ);
  long loopTime = (long)((durance * 1000) / (delayAmount * 2));

  for ( int x = 0; x < loopTime; x++) {
    ledcWriteTone(channel, 3000);
    delayMicroseconds(delayAmount);
    ledcWriteTone(channel, 0);
    delayMicroseconds(delayAmount);
  }

  delay(20);
}

void imperial_march() {
  beep(channel, a, 500);
  beep(channel, a, 500);
  beep(channel, a, 500);
  beep(channel, f, 350);
  beep(channel, cH, 150);

  beep(channel, a, 500);
  beep(channel, f, 350);
  beep(channel, cH, 150);
  beep(channel, a, 1000);
  //first bit

  beep(channel, eH, 500);
  beep(channel, eH, 500);
  beep(channel, eH, 500);
  beep(channel, fH, 350);
  beep(channel, cH, 150);

  beep(channel, gS, 500);
  beep(channel, f, 350);
  beep(channel, cH, 150);
  beep(channel, a, 1000);
  //second bit...

  beep(channel, aH, 500);
  beep(channel, a, 350);
  beep(channel, a, 150);
  beep(channel, aH, 500);
  beep(channel, gSH, 250);
  beep(channel, gH, 250);

  beep(channel, fSH, 125);
  beep(channel, fH, 125);
  beep(channel, fSH, 250);
  delay(250);
  beep(channel, aS, 250);
  beep(channel, dSH, 500);
  beep(channel, dH, 250);
  beep(channel, cSH, 250);
  //start of the interesting bit

  beep(channel, cH, 125);
  beep(channel, b, 125);
  beep(channel, cH, 250);
  delay(250);
  beep(channel, f, 125);
  beep(channel, gS, 500);
  beep(channel, f, 375);
  beep(channel, a, 125);

  beep(channel, cH, 500);
  beep(channel, a, 375);
  beep(channel, cH, 125);
  beep(channel, eH, 1000);
  //more interesting stuff (this doesn't quite get it right somehow)

  beep(channel, aH, 500);
  beep(channel, a, 350);
  beep(channel, a, 150);
  beep(channel, aH, 500);
  beep(channel, gSH, 250);
  beep(channel, gH, 250);

  beep(channel, fSH, 125);
  beep(channel, fH, 125);
  beep(channel, fSH, 250);
  delay(250);
  beep(channel, aS, 250);
  beep(channel, dSH, 500);
  beep(channel, dH, 250);
  beep(channel, cSH, 250);
  //repeat... repeat

  beep(channel, cH, 125);
  beep(channel, b, 125);
  beep(channel, cH, 250);
  delay(250);
  beep(channel, f, 250);
  beep(channel, gS, 500);
  beep(channel, f, 375);
  beep(channel, cH, 125);

  beep(channel, a, 500);
  beep(channel, f, 375);
  beep(channel, c, 125);
  beep(channel, a, 1000);
}

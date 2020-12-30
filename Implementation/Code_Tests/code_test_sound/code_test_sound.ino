#define c 261
#define d 294
#define e 329
#define f 349
#define g 391
#define gS 415
#define a 440
#define aS 466
#define b 494
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

#define Speaker_pin 26

int channel = 0;
int frequency = 4000;
int resolution = 8;

void setup() {
  Serial.begin(115200);
//  pinMode(Speaker_pin, OUTPUT);
//  digitalWrite(Speaker_pin, HIGH);
  
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Setting speaker failed!");
  }
  ledcAttachPin(Speaker_pin, channel);
}

void loop() {
  // put your main code here, to run repeatedly:
  imperial_march();
}

void tone(int channel, int frequencyHZ, long durance) {

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
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);

  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, a, 1000);
  //first bit

  tone(channel, eH, 500);
  tone(channel, eH, 500);
  tone(channel, eH, 500);
  tone(channel, fH, 350);
  tone(channel, cH, 150);

  tone(channel, gS, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);
  tone(channel, a, 1000);
  //second bit...

  tone(channel, aH, 500);
  tone(channel, a, 350);
  tone(channel, a, 150);
  tone(channel, aH, 500);
  tone(channel, gSH, 250);
  tone(channel, gH, 250);

  tone(channel, fSH, 125);
  tone(channel, fH, 125);
  tone(channel, fSH, 250);
  delay(250);
  tone(channel, aS, 250);
  tone(channel, dSH, 500);
  tone(channel, dH, 250);
  tone(channel, cSH, 250);
  //start of the interesting bit

  tone(channel, cH, 125);
  tone(channel, b, 125);
  tone(channel, cH, 250);
  delay(250);
  tone(channel, f, 125);
  tone(channel, gS, 500);
  tone(channel, f, 375);
  tone(channel, a, 125);

  tone(channel, cH, 500);
  tone(channel, a, 375);
  tone(channel, cH, 125);
  tone(channel, eH, 1000);
  //more interesting stuff (this doesn't quite get it right somehow)

  tone(channel, aH, 500);
  tone(channel, a, 350);
  tone(channel, a, 150);
  tone(channel, aH, 500);
  tone(channel, gSH, 250);
  tone(channel, gH, 250);

  tone(channel, fSH, 125);
  tone(channel, fH, 125);
  tone(channel, fSH, 250);
  delay(250);
  tone(channel, aS, 250);
  tone(channel, dSH, 500);
  tone(channel, dH, 250);
  tone(channel, cSH, 250);
  //repeat... repeat

  tone(channel, cH, 125);
  tone(channel, b, 125);
  tone(channel, cH, 250);
  delay(250);
  tone(channel, f, 250);
  tone(channel, gS, 500);
  tone(channel, f, 375);
  tone(channel, cH, 125);

  tone(channel, a, 500);
  tone(channel, f, 375);
  tone(channel, c, 125);
  tone(channel, a, 1000);
}

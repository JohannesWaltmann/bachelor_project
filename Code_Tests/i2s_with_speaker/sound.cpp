#include "sound.h"

//void tone(int channel, int frequency_in_HZ, long durance) {
//
//  long delayAmount = (long)(1000000 / frequency_in_HZ);
//  long loopTime = (long)((durance * 1000) / (delayAmount * 2));
//
//  for ( int x = 0; x < loopTime; x++) {
//    ledcWriteTone(channel, 3000);
//    delayMicroseconds(delayAmount);
//    ledcWriteTone(channel, 0);
//    delayMicroseconds(delayAmount);
//  }
//
//  delay(20);
//}
//
//void march() {
//  tone(channel, a, 500);
//  tone(channel, a, 500);
//  tone(channel, a, 500);
//  tone(channel, f, 350);
//  tone(channel, cH, 150);
//
//  tone(channel, a, 500);
//  tone(channel, f, 350);
//  tone(channel, cH, 150);
//  tone(channel, a, 1000);
//
//  tone(channel, eH, 500);
//  tone(channel, eH, 500);
//  tone(channel, eH, 500);
//  tone(channel, fH, 350);
//  tone(channel, cH, 150);
//
//  tone(channel, gS, 500);
//  tone(channel, f, 350);
//  tone(channel, cH, 150);
//  tone(channel, a, 1000);
//
//  tone(channel, aH, 500);
//  tone(channel, a, 350);
//  tone(channel, a, 150);
//  tone(channel, aH, 500);
//  tone(channel, gSH, 250);
//  tone(channel, gH, 250);
//
//  tone(channel, fSH, 125);
//  tone(channel, fH, 125);
//  tone(channel, fSH, 250);
//  delay(250);
//  tone(channel, aS, 250);
//  tone(channel, dSH, 500);
//  tone(channel, dH, 250);
//  tone(channel, cSH, 250);
//
//  tone(channel, cH, 125);
//  tone(channel, b, 125);
//  tone(channel, cH, 250);
//  delay(250);
//  tone(channel, f, 125);
//  tone(channel, gS, 500);
//  tone(channel, f, 375);
//  tone(channel, a, 125);
//
//  tone(channel, cH, 500);
//  tone(channel, a, 375);
//  tone(channel, cH, 125);
//  tone(channel, eH, 1000);
//
//  tone(channel, aH, 500);
//  tone(channel, a, 350);
//  tone(channel, a, 150);
//  tone(channel, aH, 500);
//  tone(channel, gSH, 250);
//  tone(channel, gH, 250);
//
//  tone(channel, fSH, 125);
//  tone(channel, fH, 125);
//  tone(channel, fSH, 250);
//  delay(250);
//  tone(channel, aS, 250);
//  tone(channel, dSH, 500);
//  tone(channel, dH, 250);
//  tone(channel, cSH, 250);
//
//  tone(channel, cH, 125);
//  tone(channel, b, 125);
//  tone(channel, cH, 250);
//  delay(250);
//  tone(channel, f, 250);
//  tone(channel, gS, 500);
//  tone(channel, f, 375);
//  tone(channel, cH, 125);
//
//  tone(channel, a, 500);
//  tone(channel, f, 375);
//  tone(channel, c, 125);
//  tone(channel, a, 1000);
//}

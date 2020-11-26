#include "Arduino.h"
#include "i2s.h"
#include "wav.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define I2S_MODE I2S_MODE_ADC_BUILT_IN
//#define I2S_MODE I2S_MODE_RX
const int sample_time = 40;//recording time of the sample in seconds
const char filename[]  = "/sound.wav";

const int headerSize = 44;
const int wavDataSize = sample_time * 88000;
const int numCommunicationData = 8000;
const int numPartWavData = numCommunicationData/4;

byte header[headerSize];
char communicationData[numCommunicationData];
char partWavData[numPartWavData];

File fl;

void setup() {
  Serial.begin(115200);

  if(!SD.begin()){
    Serial.println("Error while mounting SD");
    return;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD attached");
    return;
  }
  
  while (!SD.begin()){
    Serial.println(".");
    delay(500);
  }
 
  CreateWavHeader(header, wavDataSize);
  
  SD.remove(filename);
  Serial.println("Removed file");
  fl = SD.open(filename, FILE_WRITE);
  
  if (!fl) return;
  Serial.println("Writing to file");
  fl.write(header, headerSize);

  I2S_Init(I2S_MODE, I2S_BITS_PER_SAMPLE_32BIT);
  for (int i = 0; i < wavDataSize/numPartWavData; ++i) {
    I2S_Read(communicationData, numCommunicationData);
    for(int j = 0; j < numCommunicationData/8; ++j) {
      partWavData[2*j] = communicationData[8*j +2];
      partWavData[2*j +1] = communicationData[8*j +3];
    }
    fl.write((const byte*)partWavData, numPartWavData);
  }
  fl.close();
  Serial.println("finished");
}

void loop() {
}

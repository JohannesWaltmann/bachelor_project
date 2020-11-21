#include <WebServer.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <driver/i2s.h>
#include "heltec.h"
#include "time.h"
#include "notes.h"
#include "Arduino.h"
#include "stdio.h"

#define DEBUGGING

#ifdef DEBUGGING
#define DEBUG_B(...) Serial.begin(__VA_ARGS__)
#define DEBUG_P(...) Serial.println(__VA_ARGS__)
#define DEBUG_F(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_B(...)
#define DEBUG_P(...)
#define DEBUG_F(...)
#endif

String webpage;
String css_style;
bool record_trigger = false;

// Parameters for ledc Initialization
const int channel = 0; // PWM channel
const int frequency = 4000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit
const int speaker_output26 = 26; // Pin the Speaker runs over

// Parameters for SD Initialization
#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);

// Parameters for I2S Initialization
#define I2S_WS 15
#define I2S_SD 13
#define I2s_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //in seconds -> wenn angepasst kommt dann vernünftige Länge der Aufnahme raus??
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

// Parameters for NTP
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// File Parameters
File file;
char filename[64 + 1];
const int headerSize = 44; // Size for .wav-header

WebServer server(80);


void setup() {
  DEBUG_B(115200);
  DEBUG_F("\nSketchname: %s\nBuild: %s\t\tIDE: %d.%d.%d\n\n", __FILE__, __TIMESTAMP__, ARDUINO / 10000, ARDUINO % 10000 / 100, ARDUINO % 100 / 10 ? ARDUINO % 100 : ARDUINO % 10);
  displaySetup();
  
  mountSpeaker();
  i2sInit();
  Connect();
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Connects to NTP-Server to get current UNIX-Time

  if (!setFilename(filename, sizeof(filename))) {
    Serial.println("Couldn't format filepath");
  }
  SDInit();
  readHTMLData();
  
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", webpage);
  });
  server.on("/style.css", HTTP_GET, []() {
    server.send(200, "text/css", css_style);
  });

  server.on("/recording", set_record_trigger);

  // TODO: Weitere server.on für Clear SD und evtl Download

  server.begin();
  DEBUG_P("HTTP Server gestartet\n\n");
}

void loop() {
  server.handleClient();

  while (record_trigger) {
    recordSample();
    record_trigger = false;
  }
}

void set_record_trigger() {
  record_trigger = true;
}

void recordSample() {

  digitalWrite(speaker_output26, HIGH);
  // -> Pins für i2s auf High setzen (Diese in Setup dafür auf low gesetzt sein müssen)
  xTaskCreate(i2s_adc, "i2s_adc", 2 * 1024, NULL, 1, NULL); // Eventuell nur Methode aufrufen ohne den Task -> Dann auch Code innerhalb Methode bearbeiten 
  melody();
  delay(20000);
  // -> Alle Pins wieder auf Low setzen (Auch von i2s)
  digitalWrite(speaker_output26, LOW);
    
}

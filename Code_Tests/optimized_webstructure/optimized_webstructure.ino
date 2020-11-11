#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "ESPAsyncWebServer.h"
#include "Arduino.h"
#include "heltec.h"
#include "stdio.h"
#include "time.h"

#include "requirements.h"



String todo;


// Webserver Params
const char* ssid = "WaltLAN-2017";
const char* password = "20.JateC-ReguittI-TropeX.17";
AsyncWebServer server(80);


// Pinbelegung für SD-Karte
#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);

// Fileparameter
const int headerSize = 44;
char filename[64 + 1];
File file;

// Pinbelegung etc für Lautsprecher und NTP
String output26State_Speaker = "off";
const int speaker_output26 = 26;

// Parameters for ledc Initialization
const int channel = 0; // PWM channel
const int frequency = 4000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit

// NTP
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

/**
   @brief Processing Method that replaces a temporary char
    in the html file
*/
String processor(const String& val) {

  int cardSize = SD.cardSize() / (1024 * 1024);
  String cardSize_str = String(cardSize);
  String return_val = String(getUsedSpace() + " of " + cardSize_str);

  if (val == "X")
    return return_val;
  return String();
}

void setup() {
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10); // Set font and textsize
  Heltec.display->drawString(0, 0, "Connecting to ");
  Heltec.display->drawString(10, 10, ssid);

  Serial.begin(115200);

  SDInit();
  //....................................Attach ledcPWM to the pin for the speaker.........................................
  pinMode(speaker_output26, OUTPUT);
  digitalWrite(speaker_output26, LOW);

  Serial.println("Initializing Speaker...");
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);
  //......................................................................................................................

  WiFi.begin(ssid, password);
  // Wait for Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Heltec.display->drawString(20, 20, "Use the following IP:");
  Heltec.display->drawString(30, 40, WiFi.localIP().toString());

  Heltec.display->display();

  // Display Mainwebfile
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/web_data/interface.html", String(), false, processor);
  });
  // Send CSS-File for maininterface
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/web_data/style.css", "text/css");
  });

  // Initiate the Download of the requested file
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/recording_28 October, 14-25-50.wav", "application/octet-stream", true);
  });

  server.on("/recording", HTTP_GET, [](AsyncWebServerRequest * request) {
    // Methoden um Lautsprecher zu freizugeben und zu starten
    //    Heltec.display->drawString(50,0,"Start recording");
    //    Heltec.display->display();
    //    output26State_Speaker = "on";
    //    digitalWrite(speaker_output26, HIGH);
    //    melody();
    todo = "recording";
    AsyncWebServerResponse *response = request->beginResponse(SD, "/web_data/interface.html", String(), false, processor);
    request->send(response);
    //request->send(SD, "/web_data/interface.html", String(), false, processor);
  });

  server.begin();
}

void loop() {

  if (todo == "recording") {
    Heltec.display->drawString(0, 50, "Start recording");
    Heltec.display->display();
    output26State_Speaker = "on";
    digitalWrite(speaker_output26, HIGH);
    melody();
  }
}

//............................................................................................................................................


/**
   Computes a desired note based on the given params.

   @param channel - The used PWM channel
   @param frequencyHZ - The frequency used to compute the exact note
   @param durance - Used to display the beat of the note to be displayed
*/
void tone(int channel, int frequencyHZ, long durance) {

  // Computes the pitch of the note
  long delayAmount = (long)(1000000 / frequencyHZ);
  // Compute the length of the note
  long loopTime = (long)((durance * 1000) / (delayAmount * 2));

  // Not each tick of the computed notelength activate the speaker
  for ( int x = 0; x < loopTime; x++) {
    ledcWriteTone(channel, 3000);
    delayMicroseconds(delayAmount);
    ledcWriteTone(channel, 0);
    delayMicroseconds(delayAmount);
  }

  delay(20);
}

/**
   Plays a soundmelody by using the function tone() multiple times
   and altering notefrequency and notedurance each time
*/
void melody() {
  Serial.println("test print\n");
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);

  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, a, 1000);

  tone(channel, eH, 500);
  tone(channel, eH, 500);
  tone(channel, eH, 500);
  tone(channel, fH, 350);
  tone(channel, cH, 150);

  tone(channel, gS, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);
  tone(channel, a, 1000);

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
































//....................................................................................................................................................................


void SDInit() {

  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sd_spi)) {
    Serial.println("\nSD initialization failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("\nNo SD card attatched");
    return;
  }

  while (!SD.begin(SD_CS, sd_spi)) {
    Serial.println(".");
    delay(500);
  }

  //  file = SD.open(filename, FILE_WRITE);
  //  if (!file) {
  //    Serial.println("File not available");
  //  }
  //
  //  byte header[headerSize];
  //  wavHeader(header, FLASH_RECORD_SIZE);
  //
  //  file.write(header, headerSize);
}

String getUsedSpace(void) {

  int sumFileSize = 0;

  fs::File root = SD.open("/");
  if (!root) {
    Serial.println(F("Failed to open directory"));
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
  }

  fs::File file = root.openNextFile();
  while (file) {
    sumFileSize += file.size();
    file = root.openNextFile();
  }
  delay(1000);
  sumFileSize /= 1000;

  return String(sumFileSize);
}

#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "ESPAsyncWebServer.h"
#include "Arduino.h"
#include "heltec.h"
#include "stdio.h"

const char* ssid = "WaltLAN-2017";
const char* password = "20.JateC-ReguittI-TropeX.17";

File file;

AsyncWebServer server(80);


#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);


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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/web_data/interface.html", "text/html", false);
  });
  // Send CSS-File for maininterface
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/web_data/style.css", "text/css");
  });
  
  // Initiate the Download of the requested file
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/recording_28 October, 14-25-50.wav", "application/octet-stream", true);
  });

  server.begin();
}

void loop() {
}

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

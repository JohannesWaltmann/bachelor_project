#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

#include "i2s_specs.h"
#include "heltec.h"
#include "Arduino.h"
#include "stdio.h"
#include "time.h"

// Credentials of the used wireless network
const char* ssid = "";
const char* password = "";

//NTP Data
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Store the current state of the used pins
String output26State_Speaker = "off";

// Assign output variables to GPIO pins
const int speaker_output26 = 26;

// Parameters for ledc Initialization
const int channel = 0; // PWM channel
const int frequency = 4000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit

// Timer variables used for client timeouts defined in Millisecs
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

// Stuff for SD Cardreader
#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);

#define I2S_WS 15
#define I2S_SD 21
#define I2s_SCK 13
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //in seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

const int headerSize = 44;
char filename[64 + 1];

/**
 *  Generates the name for the recordingfile to be used by accessing an ntp server 
 *  and adding current time and date values to the filepath
 */
bool printTime(char *buffer, size_t buffer_size) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");    
  }

  int err = strftime(buffer, buffer_size, "/recording_%d %B, %H-%M-%S.wav", &timeinfo);
  if(err == 0) {
    Serial.println("strftime failed");
    return false;
  }
  return true;
}

void setup() {
  //Start Display
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Serial.begin(115200);

  // Initialize the output variable as output
  pinMode(speaker_output26, OUTPUT);
  digitalWrite(speaker_output26, LOW);

  digitalWrite(I2S_WS, LOW);
  digitalWrite(I2S_SD, LOW);
  digitalWrite(I2s_SCK, LOW);

  Serial.println("Initializing Speaker...");
  // Assign the output variable to ledc PWM functions
  if(!ledcSetup(channel, frequency, resolution)) {
     Serial.println("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);

  //Display Parameters
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->drawString(0,0,"Connecting to ");
  Heltec.display->drawString(10,10, ssid); 

  WiFi.begin(ssid, password);
  // Wait for Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Connects to NTP-Server to get current UNIX-Time

  if (!printTime(filename, sizeof(filename))) { //
    Serial.println("Couldn't format filepath");
  }
  Serial.print("Formatted filepath: ");
  Serial.println(filename);

  SDInit();
  
  // Print local IP address and start web server
  Serial.println("");
  Heltec.display->drawString(0,20, "WiFi connected.");
  Heltec.display->drawString(0,30, "IP address:");
  Heltec.display->drawString(10,40, WiFi.localIP().toString());

  Heltec.display->display();

  i2sInit();
  
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIO on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");

              //Heltec.display->clear();
              Heltec.display->drawString(0,50,"Starting audio");
              Heltec.display->display();
              
              output26State_Speaker = "on";
              digitalWrite(speaker_output26, HIGH);

              digitalWrite(I2S_WS, HIGH);
              digitalWrite(I2S_SD, HIGH);
              digitalWrite(I2s_SCK, HIGH);
              // If GPIO was turned on start to play the defined melody
              xTaskCreate(i2s_adc, "i2s_adc", 2 * 1024, NULL, 1, NULL);
              melody();
              Heltec.display->clear();
              Heltec.display->drawString(0,0, "Recording finished!");
              Heltec.display->display();
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State_Speaker = "off";
              digitalWrite(speaker_output26, LOW);
            //Download the written file to clientsystem
            } 
//            else if (header.indexOf("GET /dwnld") >= 0) {
//              Serial.println("Initiation file download");
//              
//              client.println("<?php");
//              client.println("$file = filename;");
//              client.println("if (file_exists($file)) {");
//              client.println("header('Content-Description: File Transfer');");
//              client.println("header('Content-Type: application/octet-stream');");
//              client.print("header('Content-Disposition: attachment; ");
//              client.print(""filename="");
//              client.print("'.basename($file).'");
//              client.println("');");
//              client.println("header('Expires: 0');");
//              client.println("header('Cache-Control: must-revalidate');");
//              client.println("header('Pragma: public');");
//              client.println("header('Content-Length: ' . filesize($file));");
//              client.println("readfile($file);");
//              client.println("exit;");
//              client.println("}");
//              client.println("?>");
//            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            // Create button for state ON and for state OFF
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26
            client.println("<p>Start Soundfile</p>");
            // If the output26State_Speaker is off, it displays the ON button       
            if (output26State_Speaker == "off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } 
            //Button to start Filedownload
            client.println("<p>Download Recordsample</p>");
            client.println("<p><a href=\"/dwnld\"><button class=\"button\">DOWNLOAD</button></a></p>");

            //Display UI-Reset Button
            client.println("<p>Reset UI</p>");
            client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");          

            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

/**
 * Computes a desired note based on the given params.
 * 
 * @param channel - The used PWM channel
 * @param frequencyHZ - The frequency used to compute the exact note 
 * @param durance - Used to display the beat of the note to be displayed
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
 * Plays a soundmelody by using the function tone() multiple times
 * and altering notefrequency and notedurance each time
 */
void melody() {
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

void SDInit() {

  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if(!SD.begin(SD_CS, sd_spi)) {
    Serial.println("SD initialization failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attatched");
    return;
  }

  while (!SD.begin(SD_CS, sd_spi)) {
    Serial.println(".");
    delay(500);
  }

  SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);
  if(!file){
    Serial.println("File not available");
  }

  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);
  
  file.write(header, headerSize);
  listSD();
}

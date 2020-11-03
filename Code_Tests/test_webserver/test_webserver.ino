#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>
#include "ESPAsyncWebServer.h" 
#include "requirements.h"
#include "heltec.h"
#include "Arduino.h"
#include "stdio.h"
#include "time.h"

// Credentials of the used wireless network
const char* ssid = "WaltLAN-2017";
const char* password = "20.JateC-ReguittI-TropeX.17";

File file;

//NTP Data
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Set web server port number to 80
AsyncWebServer server(80);

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

const int headerSize = 44; // Size for .wav-header

char filename[64 + 1];
/**
    Generates the name for the recordingfile to be used by accessing an ntp server
    and adding current time and date values to the filepath
*/
bool setFilename(char *buffer, size_t buffer_size) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }
  // Print the desired time and date values to a buffer as string
  int err = strftime(buffer, buffer_size, "/recording_%d %B, %H-%M-%S.wav", &timeinfo);
  if (err == 0) {
    Serial.println("strftime failed");
    return false;
  }
  return true;
}

void setup() {
  //Start Display
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);
  Heltec.display->flipScreenVertically();

  Serial.begin(115200);

  // Declare the pin for the speaker as output and deactivate it
  // until needed
  pinMode(speaker_output26, OUTPUT);
  digitalWrite(speaker_output26, LOW);

  // Deactivate Pins for the microphone until needed
//  digitalWrite(I2S_WS, LOW);
//  digitalWrite(I2S_SD, LOW);
//  digitalWrite(I2s_SCK, LOW);
  
  // Attach ledcPWM to the pin for the speaker
  Serial.println("Initializing Speaker...");
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);

  //Display Parameters
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10); // Set font and textsize

  Heltec.display->drawString(0, 0, "Connecting to ");
  Heltec.display->drawString(10, 10, ssid);

  WiFi.begin(ssid, password);
  // Wait for Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Connects to NTP-Server to get current UNIX-Time

  if (!setFilename(filename, sizeof(filename))) { //
    Serial.println("Couldn't format filepath");
  }
  
  SDInit();

  // Print local IP address and start web server
  Serial.println("");
  Heltec.display->drawString(0, 20, "WiFi connected.");
  Heltec.display->drawString(0, 30, "IP address:");
  Heltec.display->drawString(10, 40, WiFi.localIP().toString());

  Heltec.display->display();

  i2sInit(); //Maybe better to setup after server started?

  server.begin();
}

void loop() {
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

              Heltec.display->clear();
              Heltec.display->drawString(0, 10, "Starting recording");
              Heltec.display->display();

              output26State_Speaker = "on";
              digitalWrite(speaker_output26, HIGH);

//              digitalWrite(I2S_WS, HIGH);
//              digitalWrite(I2S_SD, HIGH);
//              digitalWrite(I2s_SCK, HIGH);
              // If GPIO was turned on start to play the defined melody
              xTaskCreate(i2s_adc, "i2s_adc", 2 * 1024, NULL, 1, NULL);
              //melody();
              //Heltec.display->clear();
              Heltec.display->drawString(0, 20, "Recording finished!");
              Heltec.display->display();
            } 
            else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State_Speaker = "off";
              digitalWrite(speaker_output26, LOW);
            }
            else if (header.indexOf("GET /dwnld") >= 0) { //Download the written file to clientsystem
              Serial.println("Initiating file download");

              client.println("HTTP/1.1 200 OK");
        
              client.println("Content-type:audio/wav");
              client.println("Content-Description: File Transfer");
              client.print("Content-Disposition: attachment; filename=");
              client.println(filename);
              client.println("Expires: 0");
              client.println("Cache-Control: must-revalidate");
              client.println("Pragma: public");
              client.print("Content-length: ");
              client.println(file.size());
              file.read();
            }

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

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2s_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

/**
   Scale up the recorded sound data to increase its volume
*/
void i2s_adc_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len) {
  uint32_t j = 0;
  uint32_t dac_value = 0;

  for (int i = 0; i < len; i += 2) {
    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 4096;
  }
}

/**
   Record any soundinput captured by the microphone
*/
void i2s_adc(void *arg) {
  int i2s_read_len = I2S_READ_LEN;
  int flash_wr_size = 0;
  size_t bytes_read;

  char* i2s_read_buff = (char*)calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*)calloc(i2s_read_len, sizeof(char));

  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

  Serial.println(" *** Start recording ***");
  while (flash_wr_size < FLASH_RECORD_SIZE) {
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    //    example_disp_buf((uint8_t*) i2s_read_buff, 64);

    i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
    file.write((const byte*) flash_write_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    ets_printf("Never used Stack size: %u\n", uxTaskGetStackHighWaterMark(NULL));
  }
  file.close();

  free(i2s_read_buff);
  i2s_read_buff = NULL;
  free(flash_write_buff);
  flash_write_buff = NULL;

  listSD();
  vTaskDelete(NULL);
}

void example_disp_buf(uint8_t* buf, int length) {
  Serial.print("=====\n");
  for (int i = 0; i < length; i++) {
    Serial.printf("%02x ", buf[i]);
    if ((i + 1) % 8 == 0) {
      Serial.printf("\n");
    }
  }
  Serial.printf("=====\n");
}

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

  if (!SD.begin(SD_CS, sd_spi)) {
    Serial.println("SD initialization failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attatched");
    return;
  }

  while (!SD.begin(SD_CS, sd_spi)) {
    Serial.println(".");
    delay(500);
  }

  //SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("File not available");
  }

  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);

  file.write(header, headerSize);
}

// https://github.com/fabiuz7/melody-player-arduino
#include "melody_player.h"
#include "melody_factory.h"
#include "requirements.h"

#include <SD.h>
#include "heltec.h"
#include "Arduino.h"
#include <driver/i2s.h>

#include <WiFi.h>
#include "time.h"
#include <SPI.h>
#include <FS.h>

#include "stdio.h"

// Params for the WiFi Connection
WiFiServer server(80); // Opens a Server on Port 80
const char* ssid = "FRITZ!Box 7590 UR";
const char* password = "98861303091966401412";
String header; // Stores the HTTP Request

// Timer variables used for client timeouts defined in Millisecs
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Params for the NTP Connection
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// MelodyPlayer output set to DAC pin
int buzzerPin = 26; 
MelodyPlayer player(buzzerPin);

File file;
char filename[64 + 1];

/**
 *  Generates the name for the recordingfile to be used by accessing an ntp server
 *  and adding current time and date values to the filepath
 */
bool setFilename(char *buffer, size_t buffer_size) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }
  // Print the desired time and date values to a buffer as string
  int err = strftime(buffer, buffer_size, "/recording_%Y %B %d, %T.wav", &timeinfo);
  if (err == 0) {
    Serial.println("strftime failed");
    return false;
  }
  return true;
}

/* Setup for the speaker
 *  Maybe not necessary due to melodyplayer
 */
const int speaker_output26 = 26; // Speaker output set to DAC pin
const int frequency = 2000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit
const int channel = 0; // PWM channel

// Pins needed to connect the SD Card
#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);

// Pins needed to configure Microphone and recording process
#define I2S_WS 15
#define I2S_SD 13
#define I2s_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //in seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

const int headerSize = 44; // Size for .wav-header

/**
 * Setup of the sketch.
 * Initializes the controllers display, establishes a connection to WLAN and to a NTP Server
 * Connects to SD Reader and Microphone.
 * Starts the Webserver.
 */
void setup() {
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);

  /* Setup for the speaker
   * Maybe not necessary due to melodyplayer
   */
  pinMode(speaker_output26, OUTPUT);

  Serial.println("Initializing Speaker...");
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);

  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, "Connecting to ");
  Heltec.display->drawString(10, 10, ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Connects to NTP-Server to get current UNIX-Time

  SDInit();

  Serial.println("");
  // Print the IP of the Controller to the display
  Heltec.display->drawString(0, 20, "WiFi connected.");
  Heltec.display->drawString(0, 30, "IP address:");
  Heltec.display->drawString(10, 40, WiFi.localIP().toString());
  Heltec.display->display();

  i2sInit();

  server.begin();
}

bool run_melody = false;
bool run_high = false;
bool run_low = false;

/**
 * Handles the webrequest and displays the website when controllers IP is called via browser.
 * Triggers the clearance of the SD Card and handles the different recording processes when called via the website.
 */
void loop() {
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length()  == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Handle GET requests 
            if (header.indexOf("GET /recordingMelody") >= 0) {
              run_melody = true;
            }
            else if (header.indexOf("GET /recordingHigh") >= 0) {
              run_high = true;
            }
            else if (header.indexOf("GET /recordingLow") >= 0) {
              run_low = true;
            }
            else if (header.indexOf("GET /clearSD") >= 0) {
              Serial.println("Wiping .wav-Data from SD");
              Heltec.display->clear();
              Heltec.display->drawString(0, 10, "Clearing Soundfiles from SD");
              Heltec.display->display();
              clearSD();
            }

            // Display the web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.print("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.print("h1{color: #0F3376;padding: 2vh;}");
            client.print(".button {display: inline-block;background-color: #008CBA; border: none;border-radius: 4px;color: white;padding: 16px 40px;text-decoration: none;font-size: 30px;margin: 2px;cursor: pointer;}");
            client.println(".button2 {background-color: #AE270B;}</style></head>");
            // Web Page Heading
            client.println("<body><h1>Microcontroller-Interface</h1>");
            client.println("<p>Start Recording</p>");
            client.println("<a href=\"/recordingHigh\"><button class=\"button\">High-Pitch</button></a>");
            client.println("<a href=\"/recordingLow\"><button class=\"button\">Low-Pitch</button></a>");
            client.println("<a href=\"/recordingMelody\"><button class=\"button\">Melody</button></a>");
            client.println("<p>Remove old Recordings</p>");
            client.println("<p><a href=\"/clearSD\"><button class=\"button button2\">Clear SD</button></a></p>");
            // 
            client.println("<dialog open>The SD currently uses " + getUsedSpace() + " Megabyte storage<br></dialog>");
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

    // Loads and plays a soundfile with mixed pitch to the MelodyPlayer and starts the recording process
    if (run_melody) {
      // Start Recording
      Serial.println("MELODY START");

      Serial.println("Loading melody...");
      String notes[] = { "C4", "G3", "G3", "A3", "G3", "SILENCE", "B3", "C4"};
      // Load and play a correct melody
      Melody melody = MelodyFactory.load("Nice Melody", 2000, notes, 8);
      player.playAsync(melody);

      Serial.println("MELODY STOP");
      Serial.println("RECORDING START");

      startRecording();
      Serial.println("RECORDING STOP");
      run_melody = false;
    }

    // Loads and plays a soundfile with a higher pitch to the MelodyPlayer and starts the recording process 
    if (run_high) {
      Serial.println("MELODY START");

      Serial.println("Loading melody...");
      String notes[] = { "C4", "G3", "G3", "A3", "G3", "SILENCE", "B3", "C4"};
      // Load and play a correct melody
      Melody melody = MelodyFactory.load("Nice Melody", 2000, notes, 8);
      player.playAsync(melody);

      Serial.println("MELODY STOP");
      run_high = false;
    }

    // Loads and plays a soundfile with a lower pitch to the MelodyPlayer and starts the recording process
    if (run_low) {
      startRecording();
      run_low = false;
    }
}
}

/**
 * Method which creates a file with the current timestamp as naming and saves it to the SD
 * Saves the .wav-Header to the File and uses the file for the recording process.
 */
void startRecording() {

  if (!setFilename(filename, sizeof(filename))) {
    Serial.println("Couldn't format filepath");
  }
  // Create File
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("File not available");
    return;
  }
  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, headerSize);

  Serial.print("\nPrinting to file: ");
  Serial.println(filename);
  Serial.print("\n");

  // Start recording of a soundsample
  i2s_adc();
}

/**
 * Standard Method to configure the Inter-IC Sound interface
 */
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
 * Method which handles the connection of the SD Card to the controller.
 * Responds to the Serial Monitor if the connection is not succesful at any point.
 */
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
  Serial.println("SD connected");
}

/**
 *  Records the input listend to by the microphone to the file currently opened.
 *  Also prints updates on the recording progession to the serial monitor.
 */
void i2s_adc(void) {
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

    Serial.print("Bytes read: ");
    Serial.println(bytes_read);

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
}

/**
 *  Scale up the recorded sound data to increase its volume.
 *  The increase is determined by the parameter 'scale_factor'
 */
void i2s_adc_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len) {
  uint32_t j = 0;
  uint32_t dac_value = 0;
  uint32_t scale_factor =  4096;

  for (int i = 0; i < len; i += 2) {
    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / scale_factor;
  }
}

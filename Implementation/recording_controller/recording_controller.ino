#include "melody_player.h"
#include "melody_factory.h"

#include "heltec.h"
#include "Arduino.h"
#include <driver/i2s.h>

#include <WiFi.h>
#include "time.h"
#include "stdio.h"

// Pins needed to configure Microphone and recording process
#define I2S_WS 15
#define I2S_SD 13
#define I2s_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (30) //in seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)


// Params for the WiFi Connection
WiFiServer server(80); // Opens a Server on Port 80
const char* ssid = "";
const char* password = "";
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
  int err = strftime(buffer, buffer_size, "recording_%Y %B %d-%H-%M-%S.wav", &timeinfo);
  if (err == 0) {
    Serial.println("strftime failed");
    return false;
  }
  return true;
}

/**
 * Setup for the speaker
 * Maybe not necessary due to melodyplayer
 */
const int speaker_output26 = 26; // Speaker output set to DAC pin
const int frequency = 2000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit
const int channel = 0; // PWM channel

const int headerSize = 44; // Size for .wav-header

char* i2s_read_buff;
uint8_t* flash_write_buff;

/**
 * Setup of the sketch.
 * Initializes the controllers display, establishes a connection to WLAN and to a NTP Server
 * Connects to SD Reader and Microphone.
 * Starts the Webserver.
 */
void setup() {
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);

  /**
   * Setup for the speaker
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

  Serial.println("");
  // Print the IP of the Controller to the display
  Heltec.display->drawString(0, 20, "WiFi connected.");
  Heltec.display->drawString(0, 30, "IP address:");
  Heltec.display->drawString(10, 40, WiFi.localIP().toString());
  Heltec.display->display();

  i2sInit();
  buffersInit();

  server.begin();
  Serial.println("Setup() done.");
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
    Serial.println("Client connected.");
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
            client.println("Access-Control-Allow-Origin: *");

            // Handle GET requests 
            if (header.indexOf("GET /recordingMelody") >= 0) {
              run_melody = true;
              break;
            }
            else if (header.indexOf("GET /recordingHigh") >= 0) {
              run_high = true;
              break;
            }
            else if (header.indexOf("GET /recordingLow") >= 0) {
              run_low = true;
              break;
            }

            // Build a string to store webpage in.
            String S = "<!DOCTYPE html><html>";
            S += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
            S += "<link rel=\"icon\" href=\"data:,\">";
            S += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
            S += "h1{color: #0F3376;padding: 2vh;}";
            S += ".button {display: inline-block;background-color: #008CBA; border: none;border-radius: 4px;color: white;padding: 16px 40px;text-decoration: none;font-size: 30px;margin: 2px;cursor: pointer;}";
            S += ".button2 {background-color: #AE270B;}</style></head>";
            // Web Page Heading
            S += "<body><h1>Microcontroller-Interface</h1>";
            S += "<p>Start Recording</p>";
            S += "<a href=\"/recordingHigh\"><button class=\"button\">High-Pitch</button></a>";
            S += "<a href=\"/recordingLow\"><button class=\"button\">Low-Pitch</button></a>";
            S += "<a href=\"/recordingMelody\"><button class=\"button\">Melody</button></a>";
            S += "</body></html>";
            // The HTTP response ends with another blank line
            S += "\n";

            client.println("Content-type: text/html");
            client.print("Content-Length: ");
            client.println(S.length() + 2); // Excess found in a non pipelined read: excess = 2, size = 960, maxdownload = 960, bytecount = 0; so we add "+2" here
            client.println("Connection: close");
            client.println();
            client.println(S);
            
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
    
    // Loads and plays a soundfile with mixed pitch to the MelodyPlayer and starts the recording process
    if (run_melody) {
      // Load and play a correct melody
      Serial.println("MELODY START");
      Serial.println("Loading melody...");
      String notes[] = {"C4", "GS3", "G5", "FS6", "AS3", "G3", "A2", "B3", "C4", "F5", "G4", "D4", "G4", "GS6", "A3", "AS6", "B2", "C4", "CS6", "D4", "DS2"};
      Melody melody = MelodyFactory.load("Mixed Pitch", 1000, notes, 21);
      Serial.println(String(" Time unit:") + melody.getTimeUnit());
      player.playAsync(melody);
      Serial.println("MELODY STOP");
     
      Serial.println("RECORDING START");
      startRecording2(&client);
      Serial.println("RECORDING STOP");
      run_melody = false;
    } else 
    // Loads and plays a soundfile with a higher pitch to the MelodyPlayer and starts the recording process 
    if (run_high) {
      Serial.println("HIGHPITCH_START");
      String high_pitched_notes[] = {"D7", "E6", "C5", "GS6", "A7", "B5", "AS6", "D7", "E6", "C5", "GS6", "A7", "B5", "C6", "F6", "G5", "D7", "G7", "GS6", "A6", "AS6", "B7"};
      Melody high_pitch = MelodyFactory.load("high_pitch", 1000, high_pitched_notes, 21);
      player.playAsync(high_pitch);
      Serial.println("HIGHPITCH STOP");

      Serial.println("RECORDING START");
      startRecording2(&client);
      Serial.println("RECORDING STOP");
      run_high = false;
    } else 
    // Loads and plays a soundfile with a lower pitch to the MelodyPlayer and starts the recording process
    if (run_low) {
      Serial.println("LOWPITCH_START");
      String low_pitched_notes[] = {"D2", "CS3", "B3", "F3", "B2", "AS2", "D2", "E3", "D3", "E2", "C3", "GS3", "A2", "B3", "C2", "GS2", "D3", "E2", "C2", "GS2", "A1", "B0"};
      Melody low_pitch = MelodyFactory.load("low_pitch", 1000, low_pitched_notes, 21);
      player.playAsync(low_pitch);
      Serial.println("LOWPITCH STOP");

      Serial.println("RECORDING START");
      startRecording2(&client);
      Serial.println("RECORDING STOP");
      run_low = false;
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
 * Method which uses an existing WiFiClient to establish
 * an data downstream from the opening browser.
 * The datastream contains the ressources generated and recorded by the i2s_adc2 method.
 * 
 * @param client - The client which is currently connected to the hosted webserver
 */
void startRecording2(WiFiClient* client) {
  setFilename(filename, sizeof(filename));
  client->println("Content-Type: application/octet-stream"); // octet-stream used to initiate a file download
  client->print("Content-Disposition: attachment; filename=\"");
  client->print(filename);
  client->println("\"");
  //client->println("Transfer-Encoding: chunked");
  client->println("Connection: close");
  client->println();

  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);
  client->write(header, headerSize);
  
  i2s_adc2(client);
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
 *  Records the input listend to by the microphone and sends it to the given client
 *  for the download process.
 *  Also prints updates on the recording progession to the serial monitor.
 *  
 *  @param wificlient - The currently established webclient used to download the sounddata
 */
void i2s_adc2(WiFiClient* wificlient) {
  int flash_wr_size = 0;
  size_t bytes_read;

  Serial.print("I2S_READ_LEN: ");
  Serial.println(I2S_READ_LEN);

  i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

  Serial.println(" *** Start recording ***");
  while (flash_wr_size < FLASH_RECORD_SIZE) {
    i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

    Serial.print("Bytes read: ");
    Serial.println(bytes_read);

    i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, I2S_READ_LEN);
    Serial.println("i2s_adc_data_scale returned.");
    wificlient->write((const byte*) flash_write_buff, I2S_READ_LEN); // sends each read byte of data to the WiFiClient
    flash_wr_size += I2S_READ_LEN;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    ets_printf("Never used Stack size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    bufferZero();
  }
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

/**
 * Initializes the buffers read and write buffers only once to prevent memory fragmentation.
 */
void buffersInit(){
  Serial.println("Initializing i2s_read_buff and flash_write_buff ...");
  i2s_read_buff = (char*) calloc(I2S_READ_LEN, sizeof(char));
  flash_write_buff = (uint8_t*) calloc(I2S_READ_LEN, sizeof(char));

  if(i2s_read_buff == NULL) {
    Serial.println("!!! Calloc1 failed: char* i2s_read_buff = (char*)calloc(I2S_READ_LEN, sizeof(char));");
    for(;;); // stop execution forever
  } else {
    Serial.println(ets_printf("    i2s_read_buff: %p", i2s_read_buff));
  }

  if(flash_write_buff == NULL) {
    Serial.println("!!! Calloc2 failed: uint8_t* flash_write_buff = (uint8_t*)calloc(I2S_READ_LEN, sizeof(char));");
    for(;;); // stop execution forever
  } else {
    Serial.println(ets_printf("    flash_write_buff: %p", flash_write_buff));
  }
  
  Serial.println("initialization done.");
}

/**
 * Resets both buffers to zero (0x00).
 */
void bufferZero() {
  memset(i2s_read_buff, 0x00, I2S_READ_LEN);
  memset(flash_write_buff, 0x00, I2S_READ_LEN);
}

/**
 * https://playground.arduino.cc/Code/AvailableMemory/
 */
int freeRam () {
  return ESP.getFreeHeap();
}

void printFreeRAM() {
  return; // disable printing of free ram 
  
  Serial.print("Free RAM: ");
  Serial.println(freeRam());
}

/**
 * Creates the header for a .wav-file.
 */
void wavHeader(byte* header, int wavSize) {
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize =  wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // monoral
  header[23] = 0x00;
  header[24] = 0x80;  // sampling rate 44100
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;  // Byte/sec = 44100x2x1 = 88200
  header[29] = 0x7D;
  header[30] = 0x01;
  header[31] = 0x00;
  header[32] = 0x02;  // 16bit monoral
  header[33] = 0x00;
  header[34] = 0x10;  // 16bit
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);
}

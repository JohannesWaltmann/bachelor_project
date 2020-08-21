#include <driver/i2s.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "sound.h"


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

#define SPEAKER_PIN 2

//Parameters for the soundfile
File file;
const char filename[] = "/recording.wav";
const int headerSize = 44;

//Parameters for the speaker
const int channel = 0;
const int speaker_frequency = 4000;
const int resolution  = 8;

void setup() {
  Serial.begin(115200);
  SDInit();
  i2sInit();
  SpeakerInit();
  xTaskCreate(i2s_adc, "i2s_adc", 2 * 1024, NULL, 1, NULL);
}

void loop() {
  march();
}
/**
 * Initialize and setup the Speaker
 */
void SpeakerInit() {
  if (!ledcSetup(channel, speaker_frequency, resolution)) {
    Serial.println("Setting speaker failed");
  }
  ledcAttachPin(SPEAKER_PIN, channel);
}

/**
 * Check if the SD card is available and if so
 * create a file for the soundrecording and add the .wav header to it
 */
void SDInit() {
  if(!SD.begin()) {
    Serial.println("SD initialization failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attatched");
    return;
  }

  while (!SD.begin()) {
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

/**
 * Initialize the parameters for the i2s recordings
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
 * Scale up the recorded sound data to increase its volume
 */
void i2s_adc_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len) {
  uint32_t j = 0;
  uint32_t dac_value = 0;

  for (int i = 0; i < len; i += 2){
    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 4096;
  }
}

/**
 * Record any soundinput captured by the microphone
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
  while (flash_wr_size < FLASH_RECORD_SIZE){
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
    for (int i = 0; i < length; i++){
      Serial.printf("%02x ", buf[i]);
      if ((i +1) % 8 == 0){
        Serial.printf("\n");    
      }
    }
    Serial.printf("=====\n");    
}

/**
 * The Data for the Headerfile of the .wav
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

/**
 * Used to print the number of files on the SD card including their
 * names and size
 */
void listSD(void) {
  Serial.println(F("\r\nListing SD files:"));
  static const char line[] PROGMEM =  "=================================================";

  Serial.println(FPSTR(line));
  Serial.println(F("  File name                              Size"));
  Serial.println(FPSTR(line));

  fs::File root = SD.open("/");
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      // File path can be 31 characters maximum in SPIFFS
      int spaces = 33 - fileName.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(FPSTR(line));
  Serial.println();
  delay(1000);
}

/**
 * Generates a buzzer? sound for a connected speaker 
 * 
 * @param frequency_in_HZ the pitch of each single generated note
 * @param durance The length of each individual sound
 */
void tone(int channel, int frequency_in_HZ, long durance) {

  long delayAmount = (long)(1000000 / frequency_in_HZ);
  long loopTime = (long)((durance * 1000) / (delayAmount * 2));

  for ( int x = 0; x < loopTime; x++) {
    ledcWriteTone(channel, 3000);
    delayMicroseconds(delayAmount);
    ledcWriteTone(channel, 0);
    delayMicroseconds(delayAmount);
  }

  delay(20);
}

/**
 * Puts together a melody of different notes 
 * in this case the 'imperial march'
 */
void march() {
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);

  tone(channel, a, 500);
  tone(channel, f, 350);
  tone(channel, cH, 150);
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

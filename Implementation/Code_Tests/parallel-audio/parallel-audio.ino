// https://github.com/fabiuz7/melody-player-arduino
#include "melody_player.h"
#include "melody_factory.h"

int buzzerPin = 26;

MelodyPlayer player(buzzerPin);


#include <SD.h>
#include "heltec.h"
#include "Arduino.h"
#include <driver/i2s.h>

File file;

// Assign output variables to GPIO pins
const int speaker_output26 = 26;
const int frequency = 2000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit
const int channel = 0; // PWM channel

const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 466;
const int b = 494;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

// Stuff for SD Cardreader
#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sd_spi(HSPI);

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

void setup() {
  Heltec.begin(true/*enables Display*/, false/*disables LoRa*/, true/*enables Serial*/);
  
  // put your setup code here, to run once:
  pinMode(speaker_output26, OUTPUT);
  
  Serial.println("Initializing Speaker...");
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Initialization failed..");
  }
  
  ledcAttachPin(speaker_output26, channel);

  SDInit();
  i2sInit();
  Serial.println("Start!");
}

bool run_once = false;
void loop() {
  // put your main code here, to run repeatedly:  
  if (! run_once) {
      Serial.println("MELODY START");
      //melody();  // start melody

      Serial.println("Loading melody...");
      String notes[] = { "C4", "G3", "G3", "A3", "G3", "SILENCE", "B3", "C4"};
      // Load and play a correct melody
      Melody melody = MelodyFactory.load("Nice Melody", 2000, notes, 8);
      player.playAsync(melody);
      
      Serial.println("MELODY STOP");
      Serial.println("RECORDING START");
      
      startRecording();
      Serial.println("RECORDING STOP");
      run_once=true;
  }  
}

void startRecording() {
  char* filename = "/test.wav";
  
  if (!setFilename(filename, sizeof(filename))) { //
        Serial.println("Couldn't format filepath");
      }
      // Create File
      Serial.println("Opening file:");
      Serial.println(filename);
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

void tone(int channel, int frequencyHZ, long durance) {

  // Computes the pitch of the note
  long delayAmount = (long)(1000000 / frequencyHZ);
  // Compute the length of the note
  long loopTime = (long)((durance * 1000) / (delayAmount * 2));

  // Not each tick of the computed notelength activate the speaker
  for ( int x = 0; x < loopTime; x++) {
    ledcWriteTone(channel, 2000);
    delayMicroseconds(delayAmount);
    ledcWriteTone(channel, 0);
    delayMicroseconds(delayAmount);
  }

  delay(20);
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
  Serial.println("SD connected");
  //SD.remove(filename);
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

/**
   Record any soundinput captured by the microphone
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

  //  vTaskDelete(NULL);
}

bool setFilename(char *buffer, size_t buffer_size) {
  
  return true;
}

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

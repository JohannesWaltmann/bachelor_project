#include <driver/i2s.h>
#include <SPIFFS.h>

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

File file;
const char filename[] = "/recording.wav";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPIFFSInit();
  i2sInit();
  xTaskCreate(i2s_adc, "i2s_adc", 1024 * 2, NULL, 5, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void SPIFFSInit() {
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialisation failed");
    while(1) yield();
  }

  SPIFFS.format();
  file = SPIFFS.open(filename, FILE_WRITE);
  if(!file) {
    Serial.println("File is not available");
  }

  listSPIFFS();
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

void i2s_adc(void *arg) {
  
  int i2s_read_len = I2S_READ_LEN;
  int flash_wr_size = 0;
  size_t bytes_read;

  char* i2s_read_buff = (char*)calloc(i2s_read_len, sizeof(char));
//  uint8_t* flash_write_buff = (uint8_t*)calloc(i2s_read_len, sizeof(char));
 
  Serial.println(" *** Start recording ***");
  while (flash_wr_size < FLASH_RECORD_SIZE){
    //read data from I2S bus, here from ADC
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    example_disp_buf((uint8_t*) i2s_read_buff, 64);
    //save original data into flash
    flash_wr_size += i2s_read_len;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    ets_printf("Never used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
  }
  
  free(i2s_read_buff);
  i2s_read_buff = NULL;

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


void listSPIFFS(void) {
  Serial.println(F("\r\nListing SPIFFS files:"));
  static const char line[] PROGMEM =  "=================================================";

  Serial.println(FPSTR(line));
  Serial.println(F("  File name                              Size"));
  Serial.println(FPSTR(line));

  fs::File root = SPIFFS.open("/");
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

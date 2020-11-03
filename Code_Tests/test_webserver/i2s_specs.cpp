//#include "i2s_specs.h"
//#include "SD.h"
//#include "FS.h"
//#include <driver/i2s.h>
//#include "SPI.h"
//#include "Arduino.h"
//
//#define I2S_PORT I2S_NUM_0
//#define I2S_SAMPLE_RATE   (16000)
//#define I2S_SAMPLE_BITS   (16)
//#define I2S_READ_LEN      (16 * 1024)
//#define RECORD_TIME       (20) //in seconds
//#define I2S_CHANNEL_NUM   (1)
//#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
//
//File file;
//
//void i2sInit() {
//  i2s_config_t i2s_config = {
//    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
//    .sample_rate = I2S_SAMPLE_RATE,
//    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
//    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
//    .intr_alloc_flags = 0,
//    .dma_buf_count = 64,
//    .dma_buf_len = 1024,
//    .use_apll = 1
//  };
//
//  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
//
//  const i2s_pin_config_t pin_config = {
//    .bck_io_num = I2s_SCK,
//    .ws_io_num = I2S_WS,
//    .data_out_num = -1,
//    .data_in_num = I2S_SD
//  };
//
//  i2s_set_pin(I2S_PORT, &pin_config);
//}
//
//void i2s_adc_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len) {
//  uint32_t j = 0;
//  uint32_t dac_value = 0;
//
//  for (int i = 0; i < len; i += 2){
//    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
//    d_buff[j++] = 0;
//    d_buff[j++] = dac_value * 256 / 4096;
//  }
//}
//
//void i2s_adc(void *arg) {
//   int i2s_read_len = I2S_READ_LEN;
//  int flash_wr_size = 0;
//  size_t bytes_read;
//
//  char* i2s_read_buff = (char*)calloc(i2s_read_len, sizeof(char));
//  uint8_t* flash_write_buff = (uint8_t*)calloc(i2s_read_len, sizeof(char));
//
//  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
//  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
//   
//  Serial.println(" *** Start recording ***");
//  while (flash_wr_size < FLASH_RECORD_SIZE){
//    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
//
//    i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
//    file.write((const byte*) flash_write_buff, i2s_read_len);
//    flash_wr_size += i2s_read_len;
//    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
//    ets_printf("Never used Stack size: %u\n", uxTaskGetStackHighWaterMark(NULL));
//  }
//  file.close();
//  
//  free(i2s_read_buff);
//  i2s_read_buff = NULL;
//  free(flash_write_buff);
//  flash_write_buff = NULL;
//  
//  listSD();
//  vTaskDelete(NULL);
//}

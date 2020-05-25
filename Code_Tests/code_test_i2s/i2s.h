#include <Arduino.h>
#include <drivers/i2s.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLE_RATE(44100) //in Hz
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 22
#define PIN_I2S_DIN 34
#define PIN_I2S_DOUT 25

/**
 * MODE either I2S_MODE_RX I2S_MODE_MASTER or I2S_MODE_TX
 * 
 * BPS either I2S_BITS_PER_SAMPLE_16BIT or I2S_BITS_PER_SAMPLE_32BIT
 */
void I2S_Init(i2s_mode_t MODE, i2s_bits_per_sample BPS);

/**
 * @param data
 * @param numData Size of Data
 * 
 * @return Number bytes read
 */
 int I2S_Read(char*  data, int numData);

 /**
  * @param data
  * @param numData 
  */
  void I2S_Write(char* data, int numData);

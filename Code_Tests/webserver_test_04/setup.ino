
// Activate use of Display and Serial Monitor
void displaySetup() {
  Heltec.begin(true, false, true);
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
}

// Mount SD Card
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

  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("File not available!");
  }

  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);

  file.write(header, headerSize);
}

// Print time and date value to filename
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

// Mount ledc to Speaker Port
void mountSpeaker() {
  Serial.println("Initializing Speaker...");
  if (!ledcSetup(channel, frequency, resolution)) {
    Serial.println("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);

  pinMode(speaker_output26, OUTPUT);
  digitalWrite(speaker_output26, LOW);
}

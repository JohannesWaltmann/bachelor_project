const char* ssid = "WaltLAN-2017";
const char* password = "20.JateC-ReguittI-TropeX.17";

void Connect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    DEBUG_F(".");
    if (millis() > 10000) {
      DEBUG_P("\nVerbindung zum AP fehlgeschlagen\n\n");
      ESP.restart();
    }
  }
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Verbunden mit: " + WiFi.SSID());
  Heltec.display->drawString(0, 20, "IP: " + WiFi.localIP().toString());
  Heltec.display->display();
}

void readHTMLData() {
  File fl_web = SD.open("/web_data/interface.html");
  if (fl_web) {
    while (fl_web.available()) {
      char ltr = fl_web.read();
      webpage += ltr;
    }
    fl_web.close();
  }
  File fl_css = SD.open("/web_data/style.css");
  if (fl_css) {
    while (fl_css.available()) {
      char ltr = fl_css.read();
      css_style += ltr;
    }
    fl_css.close();
  }
}

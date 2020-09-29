// Load Wi-Fi library
#include <WiFi.h>
#include "sound.h"

// Credentials of the used wireless network
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State_Speaker = "off";

// Assign output variables to GPIO pins
const int speaker_output26 = 26;
// Parameters for ledc Initialization
const int channel = 0; // PWM channel
const int frequency = 4000; // Initial frequency
const int resolution = 8; // Resolution of the duty cycle in Bit

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Initialize the output variable as output
  pinMode(speaker_output26, OUTPUT);
  // Set output to LOW
  digitalWrite(speaker_output26, LOW);
  // Assign the output variable to ledc PWM functions
  Serial.print("Initializing Speaker...");
  if(!ledcSetup(channel, frequency, resolution)) {
     Serial.print("Initialization failed..");
  }
  ledcAttachPin(speaker_output26, channel);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // Wait for Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
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
              output26State_Speaker = "on";
              digitalWrite(speaker_output26, HIGH);
              // If GPIO was turned on start to play the defined melody
              melody();
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State_Speaker = "off";
              digitalWrite(speaker_output26, LOW);
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
            client.println("<p>GPIO 26 - State " + output26State_Speaker + "</p>");
            // If the output26State_Speaker is off, it displays the ON button       
            if (output26State_Speaker == "off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
            // If output26State_Speaker is on, it displays the OFF button
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
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

/**
 * Computes a desired note based on the given params.
 * 
 * @param channel - The used PWM channel
 * @param frequencyHZ - The frequency used to compute the exact note 
 * @param durance - Used to display the beat of the note to be displayed
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
 * Plays a soundmelody by using the function tone() multiple times
 * and altering notefrequency and notedurance each time
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

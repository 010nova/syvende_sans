#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Wire.h"
#include "thingProperties.h"
const char* ssid = "***";
const char* password = "***";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  Serial.begin(9600);

  // kobler seg på nett
  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  // initialiserer og synkroniserer klokke
  timeClient.begin();
  timeClient.setTimeOffset(2 * 3600); // setter riktig tidssone
  timeClient.forceUpdate();

  // Defined in thingProperties.h
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you'll get.
     The default is 0 (only errors).
     Maximum is 4
  */

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Start the I2C Bus as Slave on address 9.
  // The sender of the data also has this address in its code to communicate
  Wire.begin(9);
  // Attach a function to trigger when something is received.
  Wire.onReceive(receiveEvent);
}

void loop() {
  ArduinoCloud.update();
  sendCurrentTime();
  // Here we just wait, because this increases the stability of this loop
  delay(1000);;
}

void sendCurrentTime() {
  timeClient.update();

  // Henter tid
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  // Printer tiden
  Serial.printf("%02d:%02d\n", hours, minutes);
}

// This is a callback function for receiving data via I2C from the measuring arduino
void receiveEvent(int bytes) {
  Serial.println("test:");
  // First receive the signal char
  char sig = Wire.read();

  // We receive the floating point value as bytes, and assemble them into a float
  // For this purpose we represent the float as a byte pointer, and place the incoming
  // bytes consecutively inside the float. Finally we can use it as a float again
  float t;
  byte * p = (byte*) &t;
  unsigned int i;
  for (i = 0; i < sizeof t; i++) *p++ = Wire.read();

  String prefix = "{ \"Temperatur vannbad\": \"";
  String midfix = "\",\"Temperatur steke\": \"";
  String suffix = "\" }";

  // Now, we are going to send the data over to google sheets as a string so we
  // convert the float to a string of minimum length 4, and 2 numbers after the dot
  char temp_arr[16];
  dtostrf(t, 4, 2, temp_arr);
  if(sig=='V') data = prefix+String(temp_arr)+midfix+""+suffix;
  else if(sig=='S') data = prefix+""+midfix+String(temp_arr)+suffix;
  
  Serial.println(data);
}

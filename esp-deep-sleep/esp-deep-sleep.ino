
#include <ESP8266WiFi.h>
 
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) {
  }
  
  Serial.println("I'm awake.");

  Serial.println("Going into deep sleep for 20 seconds");
  Serial.flush();

  delay(100);
  ESP.deepSleep(5e6); // 20e6 is 20 microseconds
  delay(100);
}

void loop() {
}

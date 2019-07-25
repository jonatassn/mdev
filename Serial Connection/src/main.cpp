/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 *
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 *
*/

#include <Arduino.h>

#define RXD2 16
#define TXD2 17
#define LUZ 18

void setup() {
  delay(10000);
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));

  pinMode(LUZ, OUTPUT);
}

void loop() { //Choose Serial1 or Serial2 as required
  char c;
  Serial2.flush();
  if(Serial2.available()) {

    c = char(Serial2.read());
    if(c == '1') {
        digitalWrite(LUZ, HIGH);
        while (Serial2.available()) {
            Serial.print(char(Serial2.read()));
        }
    }
  }
  else {
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  }
  Serial.println(" LACO RECEBE");
  delay(2000);
  digitalWrite(LUZ, LOW);
  delay(2000);

}

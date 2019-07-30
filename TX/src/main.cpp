/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 *
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 *
*/

#include <Arduino.h>
//pinos RX2 TX2
#define RXD2 16
#define TXD2 17

//macsniffer
#include <time.h>
#include <sys/time.h>

#include <WiFi.h>
#include <Wire.h>

#include "esp_wifi.h"

#include "SSD1306.h"
#include "SH1106.h"

String maclist[64][4];
//maclist[n][0] - MACADDRESS
//maclist[n][1] - TTL
//maclist[n][2] - STATUS
//maclist[n][3] - DATA/HORA CAPTURA
int listcount = 0;

// Nome - MAC => Dispositivos que queremos reconhecer
String KnownMac[10][2] = {
  {"Gil-Phone","B85077E8E279"},
  {"Will-PC","E894Fffffff3"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"}

};

// Tempo máximo passado antes do dispositivo ser considerado offline
String defaultTTL = "20";

const wifi_promiscuous_filter_t filt={ //Idk what this does
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct { // still dont know much about this
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

struct tm data;

#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14

int curChannel = 1;

String getData() {
  time_t tt = time(NULL);
  data = *gmtime(&tt);

  char data_formatada[64];
  strftime(data_formatada, 64, "%d/%m/%Y %H:%M:%S", &data);
  return String(data_formatada);
}

// This is where packets end up after they get sniffed
void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf; // Dont know what these 3 lines do
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  len -= sizeof(WifiMgmtHdr);

  if (len < 0){
    return;
  }

  String packet;
  String mac;
  String mac_format = "";
  int fctl = ntohs(wh->fctl);
  for(int i=8;i<=8+6+1;i++){ // This reads the first couple of bytes of the packet. This is where you can read the whole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
      if(String(p->payload[i],HEX).length() != 2) {
          packet += "0";
          packet += String(p->payload[i],HEX);
      } else{
          packet += String(p->payload[i],HEX);
      }

  }

  for(int i=4;i<=15;i++){ // This removes the 'nibble' bits from the stat and end of the data we want. So we only get the mac address.
    mac += packet[i];
    if(i>= 4 && i%2 != 0 && i < 15)
      mac += ":";
  }
  mac.toUpperCase();

  int added = 0;
  for(int i=0;i<=63;i++){ // checks if the MAC address has been added before
    if(mac == maclist[i][0]){
      maclist[i][1] = defaultTTL;
      if(maclist[i][2] == "OFFLINE"){
        maclist[i][2] = "0";
        String macData ="";
        macData = macData + macList[i][0] + ","+ macList[i][1] + ","+ macList[i][2] + ","+ macList[i][3];
        sendSerial(macData);
      }
      added = 1;
    }
  }

  if(added == 0){ // If its new. add it to the array.
    maclist[listcount][0] = mac;
    maclist[listcount][1] = defaultTTL;
    maclist[listcount][3] = getData();
    Serial.println(mac);
    listcount ++;
    if(listcount >= 64){
      Serial.println("Too many addresses");
      listcount = 0;
    }
  }
}

void setup() {
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));

  /* setup wifi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);

  timeval tv;
  tv.tv_sec = 1559673943;
  settimeofday(&tv, NULL);


  Serial.println("Sniffer starting!");
}

void purge(){ // This maanges the TTL
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      int ttl = (maclist[i][1].toInt());
      ttl --;
      if(ttl <= 0){
        //Serial.println("OFFLINE: " + maclist[i][0]);
        maclist[i][2] = "OFFLINE";
        maclist[i][1] = defaultTTL;
      }else{
        maclist[i][1] = String(ttl);
      }
    }
  }
}

void updatetime(){ // This updates the time the device has been online for
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      if(maclist[i][2] == "")maclist[i][2] = "0";
      if(!(maclist[i][2] == "OFFLINE")){
          int timehere = (maclist[i][2].toInt());
          timehere ++;
          maclist[i][2] = String(timehere);
      }

      //Serial.println(maclist[i][0] + " : " + maclist[i][2]);

    }
  }
}

void showpeople(){ // This checks if the MAC is in the reckonized list and then displays it on the OLED and/or prints it to serial.
  String forScreen = "";
  for(int i=0;i<=63;i++){
    String tmp1 = maclist[i][0];
    if(!(tmp1 == "")){
      Serial.print(tmp1 + " : " + maclist[i][3] + "\n");
      for(int j=0;j<=9;j++){
        String tmp2 = KnownMac[j][1];
        if(tmp1 == tmp2){
          forScreen += (KnownMac[j][0] + " : " + maclist[i][2] + "\n");
          //Serial.print(KnownMac[j][0] + " : " + tmp1 + " : " + maclist[i][2] + "\n -- \n");
        }
      }
    }
  }
  // update_screen_text(forScreen);
}

void sendSerial(String macData) {
  if (Serial2.available()) {
    Serial2.write(macData);
    delay(2000);
    Serial.println("tx"); //debug
  }
}

void loop() { //Choose Serial1 or Serial2 as required
  //Serial.println("Changed channel:" + String(curChannel));
    if(curChannel > maxCh){
      curChannel = 1;
    }
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
    delay(1000);
    updatetime();
    purge();
    showpeople();
    curChannel++;

}

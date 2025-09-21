#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "config.h"

// Pin definitions
#define LED_PIN 5
#define BUTTON_PIN 12
#define NUM_LEDS 61
#define SPEAKER_PIN 13

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// WebSocket server
WebSocketsServer webSocket(81);

// Telegram bot
WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);

// LED slots (7 days × 3 times × 3 LEDs per slot)
int slots[7][3][3] = {
  {{0,1,2},{37,38,39},{40,41,42}},
  {{3,4,5},{36,35,34},{43,44,45}},
  {{6,7,8},{33,32,31},{46,47,48}},
  {{9,10,11},{30,29,28},{49,50,51}},
  {{12,13,14},{27,26,25},{52,53,54}},
  {{15,16,17},{24,23,22},{55,56,57}},
  {{18,19,20},{21,22,23},{58,59,60}}
};

// Status table
String status[7][3] = {{"","",""},{"","",""},{"","",""},{"","",""},{"","",""},{"","",""},{"","",""}};

void setup() {
  Serial.begin(115200);

  // Connect Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Blynk
  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  strip.begin();
  strip.show();

  Serial.println("Setup Complete");
}

void loop() {
  webSocket.loop();
  Blynk.run();
  checkSlots();
}

// Loop through all days & time slots
void checkSlots() {
  for(int day=0; day<7; day++){
    for(int slot=0; slot<3; slot++){
      runSlot(day, slot);
      delay(15000); // 15s between slots
    }
  }
}

void runSlot(int day, int timeSlot){
  int* leds = slots[day][timeSlot];
  unsigned long startTime = millis();
  digitalWrite(SPEAKER_PIN, HIGH);

  while(millis() - startTime < 10000){
    // LED colors: 0=Morning(green),1=Noon(blue),2=Night(red)
    if(timeSlot==0) setLeds(leds,0,255,0);
    else if(timeSlot==1) setLeds(leds,0,0,255);
    else setLeds(leds,255,0,0);

    delay(500);
    setLeds(leds,0,0,0);
    delay(500);

    if(digitalRead(BUTTON_PIN)==LOW){
      status[day][timeSlot] = "Medicine Taken";
      digitalWrite(SPEAKER_PIN, LOW);
      updateSerial();
      sendTelegram(day,timeSlot,"Medicine Taken");
      return;
    }
  }

  status[day][timeSlot] = "Medicine Missed";
  digitalWrite(SPEAKER_PIN, LOW);
  updateSerial();
  sendTelegram(day,timeSlot,"Medicine Missed");
}

void setLeds(int* leds,int r,int g,int b){
  for(int i=0;i<3;i++){
    strip.setPixelColor(leds[i], strip.Color(r,g,b));
  }
  strip.show();
}

void updateSerial(){
  Serial.println("\n--------------------------------------------");
  Serial.println("Day        Morning          Noon            Night");
  Serial.println("--------------------------------------------");

  const char* days[]={"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};

  for(int d=0;d<7;d++){
    Serial.print(days[d]);
    Serial.print("   ");
    for(int t=0;t<3;t++){
      int len = status[d][t].length();
      for(int i=0;i<(15-len);i++) Serial.print(" ");
      Serial.print(status[d][t]);
      Serial.print("   ");
    }
    Serial.println();
  }
  Serial.println("--------------------------------------------");
  Serial.flush();
}

void sendTelegram(int day,int slot,String msg){
  String days[]={"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
  String times[]={"Morning","Noon","Night"};
  String message = "Day: " + days[day] + " | Time: " + times[slot] + " | Status: " + msg;
  bot.sendMessage(TELEGRAM_CHAT_ID,message,"");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length){
  if(type==WStype_TEXT){
    if(strcmp((char*)payload,"fetch_data")==0){
      String data="ESP32 Data: "+String(millis());
      webSocket.sendTXT(num,data);
    }
  }
}


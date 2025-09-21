#include <Adafruit_NeoPixel.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>  // Required for ESP32

// ------------------ WiFi Credentials ------------------
const char* ssid     = "xxxxx";
const char* password = "xxxxx";

// ---------------- Telegram Credentials ----------------
#define BOT_TOKEN "xxxx"
#define CHAT_ID   "xxxxx"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
const char* chatID = CHAT_ID;

// ------------------ Hardware Pins ------------------
#define LED_PIN      5
#define BUTTON_PIN   12
#define SPEAKER_PIN  13
#define NUM_LEDS     42

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Each day has 3 slots: Morning, Noon, Night
const uint8_t slotPairs[21][2] = {
  { 0,  1}, {26, 27}, {28, 29},  // Sun
  { 2,  3}, {24, 25}, {30, 31},  // Mon
  { 4,  5}, {22, 23}, {32, 33},  // Tue
  { 6,  7}, {20, 21}, {34, 35},  // Wed
  { 8,  9}, {18, 19}, {36, 37},  // Thur
  {10, 11}, {16, 17}, {38, 39},  // Fri
  {12, 13}, {14, 15}, {40, 41}   // Sat
};

String status[21];
bool alertSent[21];  // Track whether the alert was already sent
uint8_t currentSlot = 0;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 21; i++) {
    status[i] = "";
    alertSent[i] = false;
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  strip.begin();
  strip.show();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  secured_client.setInsecure(); // Skip cert verification

  Serial.println("SmartMed Dispenser Started...");
}

void loop() {
  runSlot(currentSlot);
  currentSlot = (currentSlot + 1) % 21;
  delay(15000);  // Wait for 15 seconds
}

void runSlot(uint8_t idx) {
  unsigned long start = millis();
  digitalWrite(SPEAKER_PIN, HIGH);
  uint8_t phase = idx % 3;

  while (millis() - start < 10000) {
    if (phase == 0)       setLeds(slotPairs[idx], 0, 255,   0);   // Green
    else if (phase == 1)  setLeds(slotPairs[idx], 0,   0, 255);   // Blue
    else                  setLeds(slotPairs[idx], 255, 0,   0);   // Red

    delay(500);
    setLeds(slotPairs[idx], 0, 0, 0);  // OFF
    delay(500);

    if (digitalRead(BUTTON_PIN) == LOW) {
      status[idx] = "✔️";
      digitalWrite(SPEAKER_PIN, LOW);
      updateSerialMonitor();
      return;
    }
  }

  status[idx] = "❌";
  digitalWrite(SPEAKER_PIN, LOW);
  checkForAlert(idx);  // Check for missed doses alert
  updateSerialMonitor();
}

void checkForAlert(uint8_t idx) {
  // Check if the current and the previous slots (across consecutive days) have both been missed
  if (idx > 0 && status[idx] == "❌" && status[idx - 1] == "❌" && !alertSent[idx]) {
    String msg = "⚠️ <b>ALERT:</b> Kindly check with the patient, medicine has been missed <b>twice consecutively!</b>";
    bot.sendMessage(chatID, msg, "HTML");
    alertSent[idx] = true;  // Prevent duplicate alerts
  }
}

void setLeds(const uint8_t idxs[2], int r, int g, int b) {
  for (int i = 0; i < 2; i++) {
    strip.setPixelColor(idxs[i], strip.Color(r, g, b));
  }
  strip.show();
}

String centerAlign(const String &text, uint8_t width) {
  uint8_t len = text.length();
  if (len >= width) return text;
  uint8_t totalPad = width - len;
  uint8_t leftPad  = totalPad / 2;
  uint8_t rightPad = totalPad - leftPad;
  String s = "";
  for (uint8_t i = 0; i < leftPad;  i++) s += ' ';
  s += text;
  for (uint8_t i = 0; i < rightPad; i++) s += ' ';
  return s;
}

void updateSerialMonitor() {
  const uint8_t dayW = 14;
  const uint8_t slotW = 20;

  Serial.println();
  Serial.print(centerAlign("Day", dayW));
  Serial.print(centerAlign("Morning", slotW));
  Serial.print(centerAlign("Noon", slotW));
  Serial.print(centerAlign("Night", slotW));
  Serial.println();
  Serial.println(String(dayW + 3 * slotW, '-'));

  const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat" };

  for (uint8_t d = 0; d < 7; d++) {
    Serial.print(centerAlign(days[d], dayW));
    for (uint8_t t = 0; t < 3; t++) {
      String stat = status[d * 3 + t];
      if (stat.length() == 0) stat = "-";
      Serial.print(centerAlign(stat, slotW));
    }
    Serial.println();
  }

  Serial.println(String(dayW + 3 * slotW, '-'));
  Serial.println();
  Serial.flush();

  sendTelegramStatus();  // Send to Telegram too
}

void sendTelegramStatus() {
  const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

  String msg = "<b>💊 SmartMed Status</b>\n\n";
  msg += "<pre>";
  msg += "Day          Morn       Noon       Night\n";
  msg += "----------------------------------------\n";

  for (int d = 0; d < 7; d++) {
    String line = "";
    line += String(days[d]);
    while (line.length() < 13) line += " ";

    for (int t = 0; t < 3; t++) {
      String stat = status[d * 3 + t];
      if (stat == "✔️") stat = "✅";
      else if (stat == "❌") stat = "❌";
      else stat = "➖";

      line += stat;
      while (line.length() < 13 + (t + 1) * 10) line += " ";
    }

    msg += line + "\n";
  }

  msg += "----------------------------------------\n";
  msg += "</pre>";

  bool sent = bot.sendMessage(chatID, msg, "HTML");
  if (sent) {
    Serial.println("✅ Telegram message sent successfully.");
  } else {
    Serial.println("❌ Failed to send Telegram message.");
  }
}

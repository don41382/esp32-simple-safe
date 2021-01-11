#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <string> 

#include <ThreeWire.h>  
#include <RtcDS1302.h>

#include <WiFiUdp.h>
#include <NTPClient.h>

#include "driver/adc.h"
#include "lib/safe.h"
#include "lib/beepsound.h"
#include "credentials.h"

#define NAME_ADDRESS 0
#define START_CLOSE_PARAM "start"
#define OPEN_TIME_FIELD "open-time"

#define PIN_START_BUTTON GPIO_NUM_39
int startbuttonpressedCount = 0;

#define BUZZER_CHANNEL 0 // beep channel
#define PIN_BUZZER 17 // buzzer pin
#define PIN_LOCKER 22
#define PIN_CLOSED_BUTTON 15

#define PIN_RTC_POWER 14
#define PIN_CLK 27
#define PIN_DATA 26
#define PIN_RST 25

const time_t NO_TIME_SET = -1;

volatile bool safeOpen = false;

int unlockCode = 0;
const long todayTime = 1587711943;

BeepSound beep(BUZZER_CHANNEL);
WebServer server(80);
Preferences preferences;

ThreeWire myWire(PIN_DATA,PIN_CLK,PIN_RST);
RtcDS1302<ThreeWire> rtc(myWire);

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

time_t loadSafeOpenTime() {
  return preferences.getULong(OPEN_TIME_FIELD, -1);
}

void saveOpenTime(time_t time) {
  preferences.putULong(OPEN_TIME_FIELD,time);
}

void openSafe() {
  Serial.println("open safe");
  preferences.remove(OPEN_TIME_FIELD);
  digitalWrite(PIN_LOCKER, HIGH);
  beep.open();
}

void closeSafe() {
  Serial.println("close safe");
  safeOpen = false;
  digitalWrite(PIN_LOCKER, LOW);
  beep.close();
}

const time_t getCurrentTime() {
  if (rtc.IsDateTimeValid()) {
    return rtc.GetDateTime();
  } else {
    Serial.println("NOT SET!");
    return NO_TIME_SET;
  }
}

void handleSafeStatus() {
  if (safeOpen) {
    server.send(200, "text/html", "<h1>Safe is open</h1> <p>No time is set. Click on <a href='/start'>Start</a> to activate closing until next Friday.</p>");
  } else {
    char msg[500];
    time_t safeOpenTime = loadSafeOpenTime();
    time_t currentTime = getCurrentTime();
    String safeOpenTimeStr = ctime(&safeOpenTime);
    String currentTimeStr = ctime(&currentTime);
    snprintf(msg, 500, "<h1>safe is closed</h1><div><p>Today is %s</p> <p>open time %s</p></div>", currentTimeStr.c_str(), safeOpenTimeStr.c_str());
    server.send(200, "text/html ", msg);
  }
}

void handleUnlockCodeOpenSafe() {
  openSafe();
  server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"1; URL=/\"></head></html>");
  delay(500);
  ESP.restart();
}

void closeSafeUntilFriday() {
  bool lockInOpenPosition = ( digitalRead(PIN_CLOSED_BUTTON) == LOW);
  if (!safeOpen) {
    Serial.println("* safe is already locked.");
    beep.noAccess();
  } else if (!lockInOpenPosition) {
    Serial.println("* safe door lock is closed and musted be in open position before locking!");
    beep.noAccess();
  } else {
    time_t newTime = moveDateToFriday(getCurrentTime());
    saveOpenTime(newTime);
    closeSafe();
  }
}

void handleCloseSafeUntilFriday() {
  closeSafeUntilFriday();
  handleSafeStatus();
}

void sleep() {
  digitalWrite(PIN_LOCKER, LOW);
  Serial.flush();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  adc_power_off();
  Serial.println("* power off");
  delay(1000);
  esp_deep_sleep_start();
}

void powerOff() {
  beep.goodbye();
  sleep();
}

void updateRtc() {
  if (ntp.forceUpdate()) {
    Serial.println("* update from ntp");
    time_t now = ntp.getEpochTime();
    rtc.SetDateTime(now);
    Serial.printf("* set RTC to: %s",ctime(&now));
  } else {
    Serial.println("* coult not update NTP");
    beep.error();
    sleep();
  }

}

void IRAM_ATTR startButtonPressed() {
  Serial.println("* start button pressed");
  startbuttonpressedCount++;
}

void setup(void) {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RTC_POWER, OUTPUT);
  pinMode(PIN_LOCKER, OUTPUT);
  pinMode(PIN_START_BUTTON, INPUT);
  pinMode(PIN_CLOSED_BUTTON, PULLUP);
  digitalWrite(PIN_RTC_POWER, HIGH);


  // generate unlock code
  randomSeed(analogRead(0));
  unlockCode = random(99999);

  // init buzzer / ntp
  ledcSetup(BUZZER_CHANNEL, 500, 8);
  ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);
  
  // set time store/preferences
  preferences.begin("tresor", false);

  // remove preferences on reset
  // preferences.remove(OPEN_TIME_FIELD);

  // enable deep sleep¨
  esp_sleep_enable_ext0_wakeup(PIN_START_BUTTON, 0);

  // init wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  WiFi.setHostname("tresor");

  Serial.printf("Tresor v1.0 - unlock code %d\n",unlockCode);

  int count = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    
    if (count++ > 100) {
      beep.error();
      sleep();
    }
  }
  beep.upAndRunning();
  delay(500);
 
  if (MDNS.begin("esp32")) {
    Serial.println("* MDNS responder started");
  }

  // time
  ntp.begin();
      
  time_t openSafeTime = loadSafeOpenTime();
  time_t currentTime = getCurrentTime();

  Serial.println("Opening Hours");
  Serial.printf("* curr-time is: %s",ctime(&currentTime));
  Serial.printf("* open-time is: %s",ctime(&openSafeTime));
  
  if (currentTime == NO_TIME_SET || currentTime < todayTime) {
    Serial.println("* time not set or invalid");
    updateRtc();
    safeOpen = true;
  } else {
    Serial.println("* time is set correctly, checking if it due ...");
    safeOpen = difftime(openSafeTime,currentTime) < 0;
  }

  // start server
  server.begin();

  if (safeOpen) {
    Serial.println("* safe is open");
    openSafe();
    updateRtc();
  } else {
    Serial.println("* access denied");
    beep.noAccess();
  }

  attachInterrupt(PIN_START_BUTTON, startButtonPressed, HIGH);

  server.on((String("/") + String(unlockCode)).c_str(), handleUnlockCodeOpenSafe);
  server.on("/start", handleCloseSafeUntilFriday);
  server.on("/", handleSafeStatus);
}

long lastTime = millis();

void loop(void) {

  // Serial.printf("* door state: %d\n",digitalRead(PIN_CLOSED_BUTTON));
  // delay(100);

  if (startbuttonpressedCount >= 4) {
    startbuttonpressedCount = 0;
    closeSafeUntilFriday();
  } else if (lastTime + (100 * 1000) > millis()) {
    server.handleClient();
  } else {
    powerOff();
  }
}
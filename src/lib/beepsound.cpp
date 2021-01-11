#include "beepsound.h"
#include <Arduino.h>

BeepSound::BeepSound(int mchannel) {
  channel = mchannel;
}

void BeepSound::single() {
  ledcWrite(channel,10);
  delay(100);
  ledcWrite(channel,0);
}

void BeepSound::reset() {
  ledcWrite(channel,10);
  delay(100);
  ledcWrite(channel,0);
  delay(100);
  ledcWrite(channel,10);
  delay(100);
  ledcWrite(channel,0);
}

void BeepSound::upAndRunning() {
  for (int i=0; i<10; i++) {
    ledcWrite(channel, i*10);
    delay(50);
  }
  ledcWrite(channel, 0);
}

void BeepSound::open() {
  ledcWrite(channel,10);
  delay(1000);
  ledcWrite(channel,0);
}

void BeepSound::close() {
  ledcWrite(channel,10);
  delay(2000);
  ledcWrite(channel,0);
}


void BeepSound::goodbye() {
  for (int i=0; i < 3; i++) {
    ledcWrite(channel,10);
    delay(100);
    ledcWrite(channel,0);    
    delay(400);
  }
}

void BeepSound::noAccess() {
  ledcWrite(channel,20);
  delay(100);
  ledcWrite(channel,0);
  delay(100);
  ledcWrite(channel,20);
  delay(100);
  ledcWrite(channel,0);
  delay(100);  
}

void BeepSound::error() {
  ledcWrite(channel,10);
  delay(2000);
  ledcWrite(channel,0);
}
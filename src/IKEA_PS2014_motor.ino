#include <EEPROM.h>

// モーターの制御Pin
#define AIN1 02 // 黒
#define AIN2 04 // 緑
#define BIN1 05 // 赤
#define BIN2 12 // 青

// リミッターの入力Pin
#define UPPER_SW_PIN 13
#define LOWER_SW_PIN 14

// 移動方向
bool direction = 0;

// 停止間隔
uint16_t wait = 1300;

// SWの状態
volatile bool upperLimit = false;
volatile bool lowerLimit = false;

void rotate(bool);

void setup()
{
  pinMode(AIN1, OUTPUT); // モーター制御
  pinMode(AIN2, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);

  pinMode(UPPER_SW_PIN, INPUT); // 上下限リミッター
  pinMode(LOWER_SW_PIN, INPUT);
  Serial.begin(115200);
}

void loop()
{

  // スイッチを長押し中かどうか
  upperLimit = !digitalRead(UPPER_SW_PIN);
  lowerLimit = !digitalRead(LOWER_SW_PIN);
  if (upperLimit)
  {
    direction = true;
    rotate(direction);
  }
  if (lowerLimit)
  {
    direction = false;
    rotate(direction);
  }
  delayMicroseconds(wait);
}

void rotate(bool thisDir)
{
  static uint8_t sStep = 0;

  if (thisDir)
  {
    sStep = (sStep + 1) % 4;
  }
  else
  {
    sStep = (sStep + 3) % 4;
  }

  switch (sStep)
  {
  case 0: // 1010
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    break;
  case 1: // 0110
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    break;
  case 2: // 0101
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    break;
  case 3: // 1001
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    break;
  }
}

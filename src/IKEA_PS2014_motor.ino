#include <EEPROM.h>

// モーターの制御Pin
#define AIN1 33 // 黒
#define AIN2 25 // 緑
#define BIN1 26 // 赤
#define BIN2 27 // 青

// リミッターの入力Pin
#define UPPER_SW_PIN 34
#define LOWER_SW_PIN 35

// 超音波距離センサーのPin
#define ECHO_PIN 22 // Echo Pin
#define TRIG_PIN 23 // Trigger Pin

// 停止間隔
uint16_t wait = 700;

// 移動方向
volatile int8_t gDirection = 0;

// SWの状態
volatile bool upperLimit = false;
volatile bool lowerLimit = false;

void motorTask(void *pvParameters)
{
  static uint8_t sStep = 0;
  static uint8_t sState = 0;

  while (1)
  {
    if (gDirection == 0 && sState == 1)
    {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, LOW);
      sState = 0;
    }
    else
    {
      if (gDirection > 0)
      {
        sStep = (sStep + 1) % 4;
      }
      else if (gDirection < 0)
      {
        sStep = (sStep + 3) % 4;
      }
      sState = 1;

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
    delay(1); // delayMicroseconds(wait);
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT); // モーター制御Pin & 初期化
  pinMode(AIN2, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);

  pinMode(UPPER_SW_PIN, INPUT); // 上下限リミッター
  pinMode(LOWER_SW_PIN, INPUT);

  pinMode(ECHO_PIN, INPUT); // 距離センサーPin & 初期化
  pinMode(TRIG_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);

  xTaskCreatePinnedToCore(motorTask, "motorTask", 8192, NULL, 1, NULL, 0);
}

void loop()
{
  float duration = 0; //センサーの値
  float Distance = 0; //距離

  // スイッチを長押し中かどうか
  upperLimit = !digitalRead(UPPER_SW_PIN);
  lowerLimit = !digitalRead(LOWER_SW_PIN);

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(TRIG_PIN, HIGH); //超音波を出力
  delayMicroseconds(10);        //
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 5000); //センサからの入力
  if (duration == 0)
  {
    gDirection = 0;
  }
  else if (duration < 1000)
  {
    gDirection = -1;
  }
  else
  {
    gDirection = 1;
  }
  Serial.print("Duration:");
  Serial.print(duration);
  Distance = duration * 17 / 1000; // 音速を340m/sに設定
  Serial.print(" Distance:");
  Serial.print(Distance);
  Serial.println(" cm");

  delay(100);
}
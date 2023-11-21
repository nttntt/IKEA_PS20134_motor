#define FASTLED_ESP32_I2S true
#include <ArduinoOTA.h>
//#include <BLEDevice.h>
#include <ESPmDNS.h>
#include <FastLED.h>
#include <WiFi.h>

/* Function Prototype  */
uint8_t motionControl(void);
void connectToWifi(void);
void startOTA(void);
void startMDNS(void);
void opticReaction(uint8_t);
void motorTask(void);
void solid(void);
void round(void);
void rainbow(void);
void round(void);
void rotateColor(void);
void flash(void);
void gradation(void);
void run(void);
void gradation(void);
void explosion(void);
CRGB currentColor(uint8_t);

// ルーター接続情報
#define WIFI_SSID "0856g"
#define WIFI_PASSWORD "nttnttntt"
// OTA Hostname
#define DEVICE_NAME "IKEA_PS2014"

// モーターの制御Pin
#define AIN1 16 // 黒
#define AIN2 17 // 緑
#define BIN1 5  // 赤
#define BIN2 18 // 青

// リミッターの入力Pin
#define UPPER_SW_PIN 13
#define LOWER_SW_PIN 4

// 超音波距離センサーのPin
#define ECHO_PIN 19 // Echo Pin
#define TRIG_PIN 15 // Trigger Pin

// モーションコントロール関連
#define CONTROL_THRESHOLD 800 // 上下のthreshold(cm *59)
#define LIMIT_THRESHOLD 4000  // 反応範囲
#define DETECT_TIME 300       // 操作判定を行う時間(ms)
#define INTERVAL_TIME 400     // 操作間隔時間(ms)

// LED関連
#define DATA_PIN 22
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB
#define NUM_LEDS 44
#define BRIGHTNESS 255
CRGB leds[NUM_LEDS];

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, flash, solid, gradation, rotateColor, round, run, explosion};
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// グローバル変数
volatile int8_t gDirection = 0;           // 移動方向
volatile uint8_t gVariation = 0;          // バリエーション
static uint8_t gCurrentPatternNumber = 0; // パターンナンバー

void motorTask(void *pvParameters)
{
  static uint8_t sStep = 0;
  static uint8_t sState = 0;
  bool upperLimit = digitalRead(UPPER_SW_PIN);                 // Limiterの状態
  bool lowerLimit = digitalRead(LOWER_SW_PIN);
  while (1)
  {
    if (gDirection == 0) //&& sState == 1
    {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, LOW);
      sState = 0;
    }
    else
    {
      if (gDirection > 0 && !lowerLimit)
      {
        sStep = (sStep + 1) % 4;
      }
      else if (gDirection < 0 && !upperLimit)
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
    ArduinoOTA.handle();
    delay(1);
  }
}

void setup()
{
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  gPatterns[0]();
  FastLED.show();

  //Serial.begin(115200);

  pinMode(AIN1, OUTPUT); // モーター制御Pin & 初期化
  pinMode(AIN2, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);

  pinMode(ECHO_PIN, INPUT); // 距離センサーPin & 初期化
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(UPPER_SW_PIN, INPUT_PULLUP); // 上下限リミッター
  pinMode(LOWER_SW_PIN, INPUT_PULLUP);

  connectToWifi(); // Wi-Fiルーターに接続する
  startOTA();
  startMDNS();
  
  // delay(20); // 立ち上がり超音波センサーにノイズが乗るのでウエイト　他の処理が入って時間かかるようになったら消してOK
  xTaskCreatePinnedToCore(motorTask, "motorTask", 8192, NULL, 24, NULL, 0);
}

void loop()
{
  uint8_t motionCount = 0;

  motionCount = motionControl();
  if (motionCount == 2)
  {
    gVariation += 1;
  }
  else if (motionCount == 3)
  {
    gDirection = 0;
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % (ARRAY_SIZE(gPatterns) - 1);
  }
  else if (motionCount == 5)
  {
    gCurrentPatternNumber = ARRAY_SIZE(gPatterns) - 1;
  }
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();

  delay(30);
}

uint8_t motionControl()
{
  static uint8_t sPhase = 0;          // コントロールのフェーズ
  static uint8_t sCalledCount = 0;    // 呼び出された回数//////////////////
  static uint8_t sOperatedCount = 0;  // 反応した回数
  static uint32_t previousMillis = 0; // 最後に操作した時間

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(TRIG_PIN, HIGH); // 超音波を出力
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int32_t duration = pulseIn(ECHO_PIN, HIGH, LIMIT_THRESHOLD); //センサからの入力
  bool upperLimit = digitalRead(UPPER_SW_PIN);                 // Limiterの状態
  bool lowerLimit = digitalRead(LOWER_SW_PIN);

  switch (sPhase)
  {
  // 操作待ちフェーズ
  case 0:
    if (duration)
    {
      sPhase = 1;
      sOperatedCount = 0;
      previousMillis = millis();
    }
    break;

  // モーション判別フェーズ
  case 1:
    if (!duration) // 通過モーションならタイマーをリセットして回数判定フェーズへ
    {
      opticReaction(1);

      sPhase = 2;
      sOperatedCount = 1;
      previousMillis = millis();
    }
    else if ((millis() - previousMillis) > DETECT_TIME) // 判定時間経過後操作継続中ならモーター駆動フェーズへ
    {
      sPhase = 4;
    }
    break;

  // クリック回数判定フェーズ 操作待ち
  case 2:
    if (duration) // 操作があったらリリース待ちフェーズへ
    {
      opticReaction(1);
      sPhase = 3;
      sOperatedCount++;
      previousMillis = millis();
    }
    else if ((millis() - previousMillis) > INTERVAL_TIME) // 判定時間経過まで無操作なら通過回数を返して終了
    {
      sPhase = 0;
      return sOperatedCount;
    }
    else
    {
      opticReaction(0);
    }
    break;

  // リリース待ちフェーズ
  case 3:
    if (!duration) // リリースされたらタイマーをリセットして回数判定フェーズへ戻る
    {
      opticReaction(0);
      sPhase = 2;
      previousMillis = millis();
    }
    break;

  // モーター駆動フェーズ
  case 4:
    if (!duration) // 無反応なら停止して操作ミスの猶予待ちフェーズ
    {
      gDirection = 0;
      sPhase = 5;
      previousMillis = millis();
    }
    else // 操作中で
    {
      // CONTROL_THRESHOLDよりも手を近づけていて端まで来ていなければモーター上昇
      if ((duration < CONTROL_THRESHOLD - 30) && !upperLimit)
      {
        gDirection = -1;
      }

      // CONTROL_THRESHOLDよりも手を離していて端まで来ていなければモーター下降
      else if ((duration >= CONTROL_THRESHOLD + 30) && !lowerLimit)
      {
        gDirection = 1;
      }
      // それ以外はとりあえず停止
      else
      {
        gDirection = 0;
      }
    }
    break;

  // 操作ミスの猶予待ちフェーズ
  case 5:
    if (duration) // 操作があればモーター駆動フェーズに戻る
    {
      sPhase = 4;
    }
    else if ((millis() - previousMillis) > INTERVAL_TIME) // 判定時間経過まで無操作ならモーター駆動終了
    {
      sPhase = 0;
    }
    break;
  }

  // パラメーター表示
  if (sPhase)
  {
    Serial.print(" |");
    for (uint8_t i = 0; i < 22; ++i)
    {
      if (int(duration / 100) == i)
      {
        Serial.print("O");
      }
      else
      {
        Serial.print(" ");
      }
    }

    Serial.print("|");
    Serial.print(upperLimit);
    Serial.print(" ");
    Serial.print(lowerLimit);
    Serial.print(" Phase");
    Serial.print(sPhase);
    Serial.print(" Times");
    Serial.print(sOperatedCount);
    Serial.print(" Direction");
    Serial.print(gDirection);

    Serial.println("");
  }
  return 0;
}

void opticReaction(uint8_t turnOff)
{
  if (turnOff)
  {
    FastLED.setBrightness(0);
    FastLED.show();
  }
  else
  {
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.show();
  }
}
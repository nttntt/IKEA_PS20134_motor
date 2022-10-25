#define FASTLED_ESP32_I2S true
#include <FastLED.h>

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
#define UPPER_THRESHOLD 900  // 上下のthreshold
#define LOWER_THRESHOLD 4000 // 操作の最遠
#define INITIAL_STAGE 2      // 初期エラー対策を行うステップ数
#define CONTROL_STAGE 4      // モーション確定のステップ数

// LED関連
#define DATA_PIN 22
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB
#define NUM_LEDS 32
#define BRIGHTNESS 255
CRGB leds[NUM_LEDS];

// 移動方向
volatile int8_t gDirection = 0;

uint8_t motionControl(void);

void motorTask(void *pvParameters)
{
  static uint8_t sStep = 0;
  static uint8_t sState = 0;

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
    delay(1);
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

  pinMode(UPPER_SW_PIN, INPUT_PULLUP); // 上下限リミッター
  pinMode(LOWER_SW_PIN, INPUT_PULLUP);

  pinMode(ECHO_PIN, INPUT); // 距離センサーPin & 初期化
  pinMode(TRIG_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  xTaskCreatePinnedToCore(motorTask, "motorTask", 8192, NULL, 1, NULL, 0);
}

void loop()
{
  static uint8_t hue = 0;
  uint8_t changeColor = 0;

  changeColor = motionControl();
  if (changeColor)
  {
    hue += 32;
  }
  solid(hue);
  FastLED.show();

  delay(50);
}

uint8_t motionControl()
{
  static int8_t sCounter = 0;     //カウンター
  static int8_t sDoubleClick = 0; // 色変更の操作

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(TRIG_PIN, HIGH); // 超音波を出力
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int32_t duration = pulseIn(ECHO_PIN, HIGH, LOWER_THRESHOLD); //センサからの入力
  bool upperLimit = digitalRead(UPPER_SW_PIN);                 // Limiterの状態
  bool lowerLimit = digitalRead(LOWER_SW_PIN);

  // 1.カウンターが初期段階（＝反応直後の数値の揺れの処理時間）の間は
  if (sCounter <= INITIAL_STAGE)
  {
    // 無反応なら0にもどす（誤反応の排除と信号の安定待ち）
    if (!duration)
    {
      sCounter = 0;
    }
    // 反応ありならカウンタースタート
    else
    {
      sCounter++;
      sDoubleClick = 0;
    }
  }

  // 2.どの操作のつもりか確認の時間
  else if (sCounter <= CONTROL_STAGE)
  {
    // 無反応ならダブルクリック判定モードへ
    if (!duration)
    {
      sCounter++;
      sDoubleClick = 1;
    }
    // 反応継続ならカウンターだけアップ
    else
    {
      sCounter++;
    }
  }

  // 操作段階までカウンターが進んだとき
  else if (sCounter > CONTROL_STAGE)
  {
    // 無反応なら即停止
    if (!duration)
    {
      gDirection = 0;
      sCounter = 0;
    }
    // ダブルクリック判定モード中で反応ありなら色変更
    else if (sDoubleClick && duration)
    {
      gDirection = 0;
      sCounter = 0;
      return 1;
    }
    // 上下の限界で手を離したら即停止（頻繁に使うので
    /*else if ((upperLimit || lowerLimit) && !duration)
    {
      gDirection = 0;
      sCounter = 0;
      sFirstDuration = 0;
      sDirection = 0;
    }*/
    // 手をUPPER_THRESHOLDよりも近づけていて端まで来ていなければモーター上昇
    else if ((duration < UPPER_THRESHOLD) && !upperLimit)
    {
      gDirection = -1;
    }

    // 手をUPPER_THRESHOLDよりも離していて端まで来ていなければモーター下降
    else if ((duration >= UPPER_THRESHOLD) && !lowerLimit)
    {
      gDirection = 1;
    }

    // リミッターにかかっているときは停止
    else
    {
      gDirection = 0;
    }
  }

  // パラメーター表示
  if (sCounter)
  {
    Serial.print(sCounter);
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

    Serial.print(" Direction");
    Serial.print(gDirection);

    Serial.print(" ");
    Serial.print(upperLimit);
    Serial.print(" ");
    Serial.print(lowerLimit);
    Serial.print(" duration");
    Serial.print(sDoubleClick);
    Serial.println("");
  }
  return 0;
}

void solid(uint8_t hue)
{
  // 普通の光

  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 64));
}
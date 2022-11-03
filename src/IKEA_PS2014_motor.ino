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
#define CONTROL_THRESHOLD 800 // 上下のthreshold(cm *59)
#define DETECT_THRESHOLD 4000 // 反応範囲
#define OPERATE_STEP 5        // 操作判定を行うステップ数
#define INTERVAL_STEP 8       // モーション確定のステップ数

// LED関連
#define DATA_PIN 22
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB
#define NUM_LEDS 44
#define BRIGHTNESS 255
CRGB leds[NUM_LEDS];

// 移動方向
volatile int8_t gDirection = 0;
volatile int8_t gHue = 0;

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

  pinMode(ECHO_PIN, INPUT); // 距離センサーPin & 初期化
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(UPPER_SW_PIN, INPUT_PULLUP); // 上下限リミッター
  pinMode(LOWER_SW_PIN, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  xTaskCreatePinnedToCore(motorTask, "motorTask", 8192, NULL, 1, NULL, 0);
  delay(10); // 立ち上がり超音波センサーにノイズが乗るのでウエイト　　他の処理が入って時間かかるようになったら消してOK
}
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {gradation, flash, rainbow, solid, rotateColor, round, run};
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
uint8_t gCurrentPatternNumber = 0;

void loop()
{
  uint8_t changeColor = 0;

  changeColor = motionControl();
  if (changeColor)
  {
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % (ARRAY_SIZE(gPatterns));
    // gHue += 32;
  }
  gPatterns[gCurrentPatternNumber]();
  // solid();
  FastLED.show();

  delay(50);
}

uint8_t motionControl()
{
  static uint8_t sPhase = 0;   // コントロールのフェーズ
  static uint8_t sCounter = 0; // カウンター
  static uint8_t sTimes = 0;   // 反応した回数

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(TRIG_PIN, HIGH); // 超音波を出力
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int32_t duration = pulseIn(ECHO_PIN, HIGH, DETECT_THRESHOLD); //センサからの入力
  bool upperLimit = digitalRead(UPPER_SW_PIN);                  // Limiterの状態
  bool lowerLimit = digitalRead(LOWER_SW_PIN);

  switch (sPhase)
  {
  // 操作待ちフェーズ
  case 0:
    if (duration)
    {
      sPhase = 1;
      sCounter++;
      sTimes++;
    }
    break;

  // モーション判別フェーズ
  case 1:
    sCounter++;
    if (duration)
    {
      sTimes++;
    }
    if (sCounter > OPERATE_STEP) // 判定回数に達した時
    {
      if (duration) // 操作継続中ならモーター駆動フェーズへ
      {
        sPhase = 4;
      }
      else // 反応なしなら待ち時間タイマーをセットしてダブルクリック判定フェーズへ
      {
        sPhase = 2;
        sCounter = INTERVAL_STEP;
      }
    }
    break;

  // ダブルクリック判定フェーズ 2回目待ち
  case 2:
    sCounter--;
    if (duration) // 2回目の反応があったら次のフェーズへ
    {
      sPhase = 3;
    }
    else if (!sCounter) // 時間切れならリセット
    {
      sPhase = 0;
      sCounter = 0;
      sTimes = 0;
    }
    break;

  // ダブルクリック判定フェーズ リリース待ち
  case 3:
    if (!duration)
    {
      sPhase = 0;
      sCounter = 0;
      sTimes = 0;
      return 1;
    }
    break;

  // モーター駆動フェーズ
  case 4:
    // 無反応なら停止して操作ミスの猶予待ちのカウントダウン
    if (!duration)
    {
      gDirection = 0;
      sCounter--;
      if (!sCounter)
      {
        sPhase = 0;
        sCounter = 0;
        sTimes = 0;
      }
    }
    // 操作中なら猶予カウンターリセットして
    else
    {
      sCounter = OPERATE_STEP;

      // CONTROL_THRESHOLDよりも手を近づけていて端まで来ていなければモーター上昇
      if ((duration < CONTROL_THRESHOLD) && !upperLimit)
      {
        gDirection = -1;
      }

      // CONTROL_THRESHOLDよりも手を離していて端まで来ていなければモーター下降
      else if ((duration >= CONTROL_THRESHOLD) && !lowerLimit)
      {
        gDirection = 1;
      }

      // リミッターにかかっているときは停止
      else
      {
        gDirection = 0;
      }
    }
    break;
  }

  // パラメーター表示
  if (sPhase)
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

    Serial.print("|");
    Serial.print(upperLimit);
    Serial.print(" ");
    Serial.print(lowerLimit);
    Serial.print(" Phase");
    Serial.print(sPhase);
    Serial.print(" Times");
    Serial.print(sTimes);
    Serial.print(" Direction");
    Serial.print(gDirection);

    Serial.println("");
  }
  return 0;
}

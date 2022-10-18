#include <EEPROM.h>

// モーターの制御Pin
#define AIN1 16 // 黒
#define AIN2 17 // 緑
#define BIN1 5  // 赤
#define BIN2 18 // 青

// リミッターの入力Pin
#define UPPER_SW_PIN 0
#define LOWER_SW_PIN 2

// 超音波距離センサーのPin
#define ECHO_PIN 19 // Echo Pin
#define TRIG_PIN 15 // Trigger Pin

#define MOVING_THRESHOLD 200 // 上下と静止モーションのthreshold
#define INITIAL_STAGE 3      // 初期エラー対策を行うステップ数
#define CONTROL_STAGE 9      // 操作開始のステップ数


// 移動方向
volatile int8_t gDirection = 0;

uint8_t motionControl(void);

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

  xTaskCreatePinnedToCore(motorTask, "motorTask", 8192, NULL, 1, NULL, 0);
}

void loop()
{
  uint8_t motion = motionControl();

  delay(100);
}

uint8_t motionControl()
{
  static int32_t sFirstDuration = 0; //初回の距離
  static int8_t sCounter = 0;
  static int8_t sDirection = 0; // 位置のばらつき吸収用

  int tmp_return = 0; //デバッグ表示のための変数 完成後はこれ関連はすべて消してOK

  // digitalWrite(TRIG_PIN, LOW);
  // delayMicroseconds(1);
  digitalWrite(TRIG_PIN, HIGH); //超音波を出力
  delayMicroseconds(10);        //
  digitalWrite(TRIG_PIN, LOW);
  int32_t duration = pulseIn(ECHO_PIN, HIGH, 4500); //センサからの入力
  bool upperLimit = digitalRead(UPPER_SW_PIN);      // Limiterの状態
  bool lowerLimit = digitalRead(LOWER_SW_PIN);

  // 1.カウンターが初期段階（＝反応直後の数値の揺れの処理時間）の間は
  if (sCounter <= INITIAL_STAGE)
  {
    // 無反応なら0にもどす（誤反応の排除と信号の安定待ち）
    if (!duration)
    {
      sCounter = 0;
      sFirstDuration = 0;
      tmp_return = 0;
    }
    // 反応ありならカウンタースタート
    else
    {
      sCounter++;
      sFirstDuration = duration;
      sDirection = 0;
      tmp_return = 0;
    }
  }

  // 2.どの操作のつもりか確認の時間
  else if (sCounter < CONTROL_STAGE)
  {
    // 無反応ならカウンターダウン（センサーのブレを考慮してすぐに終了にはしない）
    if (!duration)
    {
      sCounter--;
    }
    // 初回よりも閾値以上に小さければ方向カウンタダウン
    else if (duration < sFirstDuration - MOVING_THRESHOLD)
    {
      sCounter++;
      sDirection--;
    }
    // 初回よりも閾値以上に大きければ方向カウンタアップ
    else if (duration > sFirstDuration + MOVING_THRESHOLD)
    {
      sCounter++;
      sDirection++;
    }
    // 初回と閾値以内の差ならばカウンターだけアップ
    else
    {
      sCounter++;
    }
  }

  // 操作段階までカウンターが進んだとき
  else if (sCounter >= CONTROL_STAGE)
  {
    // すでにモーター稼働中で無反応なら即停止
    if (gDirection && !duration)
    {
      gDirection = 0;
      sCounter = 0;
      sFirstDuration = 0;
      sDirection = 0;
      tmp_return = 0;
    }
    // ある程度静止していたら色変更モード確定でカウンターリセット
    else if (abs(sDirection) < 3)
    {
      gDirection = 0;
      sCounter = 0;
      sFirstDuration = 0;
      sDirection = 0;
      tmp_return = 9;
    }

    // 手を最初の位置よりも近づけていて端まで来ていなければモーター上昇
    else if ((duration < sFirstDuration) && !upperLimit)
    {
      gDirection = -1;
      tmp_return = -1;
    }
    // 手を最初の位置よりも下げていて端まで来ていなければモーター下降
    else if ((duration > sFirstDuration) && !lowerLimit)
    {
      gDirection = 1;
      tmp_return = 1;
    }
  }

  if (sCounter || tmp_return)
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
    Serial.print("return");
    Serial.print(tmp_return);

    Serial.print(" Direction");
    Serial.print(sDirection);

    Serial.print(" ");
    Serial.print(upperLimit);
    Serial.print(" ");
    Serial.print(lowerLimit);

    Serial.println("");
  }
  return 0;
}
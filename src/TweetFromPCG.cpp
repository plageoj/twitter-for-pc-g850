#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <TimeLib.h>
#include <Preferences.h>

#include "NTP.cpp"
#include <TwitterLib.h>

#include <GPRINT.h>

#define PWRLED 25
#define ERRLED 33
#define BUZZER 32

WiFiMulti wifiMulti;

// ツイート取得件数
#define Display_MaxData 15

HardwareSerial HWSerial(1);
Preferences pref;
Twitter tw;
GPRINT gp;

/**
 * 与えられたツイートを描画する
 * @param *tweet ツイートデータ
 */
void renderTweet(Twitter::Tweet *tweet)
{
  gp.gprint(tweet->name);
  HWSerial.println('\xf0'); // NL
  gp.gprint(tweet->text);
  HWSerial.println('\xee'); // END
  HWSerial.flush();
}

/**
 * ネットワークを設定して再起動
 * 1行目: 1文字目を捨て、SSIDとする
 * 2行目: パスワードとする
 *
 * @param command 1行目
 */
void setNetwork(String command)
{
  pref.putString("ssid", command.substring(1));
  HWSerial.flush();
  while (!HWSerial.available())
    ;
  command = HWSerial.readStringUntil('\r');
  pref.putString("password", command);
  ESP.restart();
}

String readLineFromSerial()
{
  Serial.flush();
  while (!Serial.available())
    ;
  String ret = Serial.readStringUntil('\r');
  Serial.println(ret);
  return ret;
}

void setAuthInfo()
{
  Serial.print(F("Consumer key: "));
  pref.putString("consumer_key", readLineFromSerial());

  Serial.print(F("Consumer secret: "));
  pref.putString("consumer_secret", readLineFromSerial());

  Serial.print(F("Access token: "));
  pref.putString("access_token", readLineFromSerial());

  Serial.print(F("Access token secret: "));
  pref.putString("access_secret", readLineFromSerial());

  Serial.println(F("\nALL SET!\nNow proceeding to the normal startup…"));
}

void setup()
{
  pinMode(PWRLED, OUTPUT);
  pinMode(ERRLED, OUTPUT);
  digitalWrite(PWRLED, HIGH);
  digitalWrite(ERRLED, HIGH);

  Serial.begin(115200);
  HWSerial.begin(9600, SERIAL_8N1, GPIO_NUM_27, GPIO_NUM_14, true);

  Serial.println(ESP.getFlashChipSize());

  pref.begin("G850TW");
  Serial.print(F("Connecting "));
  Serial.print(pref.getString("ssid"));
  Serial.print(F(" with password "));
  Serial.println(pref.getString("password", ""));
  wifiMulti.addAP(pref.getString("ssid").c_str(), pref.getString("password", "").c_str());

  bool connOut = false;
  digitalWrite(PWRLED, connOut);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(200);

    if (HWSerial.available())
    {
      String command = HWSerial.readStringUntil('\r');
      if (command.startsWith("W"))
      {
        setNetwork(command);
      }
      if (command.startsWith("E"))
      {
        ESP.restart();
      }
    }
    connOut = !connOut;
    digitalWrite(PWRLED, connOut);
  }
  digitalWrite(PWRLED, LOW);
  Serial.println(F("Connected!"));

  if (Serial.available())
  {
    setAuthInfo();
  }

  getTime();
  beginUdp();

  tw.begin(
      pref.getString("consumer_key").c_str(),
      pref.getString("consumer_secret").c_str(),
      pref.getString("access_token").c_str(),
      pref.getString("access_secret").c_str());
  gp.begin();

  digitalWrite(PWRLED, HIGH);
  digitalWrite(ERRLED, LOW);
  HWSerial.println('-'); // READY を送出
}

void loop()
{
  String command;
  static Twitter::Tweet timeline[Display_MaxData];
  static uint8_t currentTweet = 0;
  while (Serial.available())
  {
    gp.gprint(Serial.readStringUntil('\n'));
  }

  if (HWSerial.available())
  {
    command = HWSerial.readStringUntil('\r');
    Serial.println(command);

    // ユーザタイムラインを取得
    if (command.startsWith("U"))
    {
      command = command.substring(1);
      tw.getUserTimeline(now(), command.c_str(), timeline);
      currentTweet = 0;
      renderTweet(timeline);
      return;
    }

    // ツイート
    if (command.startsWith("N"))
    {
      command = gp.htzConvert(command.substring(1)) + " #G850Tw";
      tw.post(now(), command.c_str());
      HWSerial.println("\xee\n-"); // END
      return;
    }

    // ツイート検索
    if (command.startsWith("/"))
    {
      command = gp.htzConvert(command.substring(1));
      tw.search(now(), command.c_str(), timeline);
      currentTweet = 0;
      renderTweet(timeline);
      return;
    }

    // 次の一件を取得
    if (command.endsWith(">"))
    {
      if (currentTweet < Display_MaxData - 1)
      {
        currentTweet++;
        renderTweet(timeline + currentTweet);
      }
      else
      {
        gp.gprint(F("取得件数外です"));
        HWSerial.println("\xee\n-"); // END
      }
      return;
    }

    // 前の一件を取得
    if (command.endsWith("<"))
    {
      if (currentTweet > 0)
      {
        currentTweet--;
        renderTweet(timeline + currentTweet);
      }
      else
      {
        gp.gprint(F("これ以前のツイートはありません"));
        HWSerial.println("\xee\n-"); // END
      }
      return;
    }

    // ネットワークを設定
    if (command.startsWith("W"))
    {
      setNetwork(command);
    }

    // バッファクリアして同期
    if (command.endsWith("-"))
    {
      HWSerial.flush();
      HWSerial.println('-');
      return;
    }
  }
}
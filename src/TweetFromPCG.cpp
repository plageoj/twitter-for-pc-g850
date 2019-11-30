#include <Arduino.h>
#include <WiFiMulti.h>
#include <TimeLib.h>
#include <Preferences.h>

#include "NTP.cpp"
#include <TwitterLib.h>
#include <GPRINT.h>
#include <KINPUT.h>

#define PWRLED 25
#define ERRLED 33
#define BUZZER 32
#define DIPSW 34

WiFiMulti wifiMulti;

// ツイート取得件数
#define Display_MaxData 15
// 最大同時変換文節
#define Max_Segments 15

HardwareSerial HWSerial(1);
Preferences pref;
Twitter tw;
GPRINT gp;
KINPUT kanji(Max_Segments);

/**
 * 与えられたツイートを描画する
 * @param *tweet ツイートデータ
 */
void renderTweet(Twitter::Tweet *tweet)
{
  gp.gprint(tweet->name);
  HWSerial.println("\xe2"); // テキストモード
  HWSerial.println(GP_NL);
  gp.gprint(tweet->text);
  HWSerial.println(GP_END);
  HWSerial.flush();

  Serial.print('@');
  Serial.println(tweet->userScreenName);
  Serial.println(tweet->tweetId);
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

/**
 * シリアルから入力があるまで待ち、１行読み取る
 * @returns 読み取った文字列
 */
String readLineFromSerial()
{
  Serial.flush();
  while (!Serial.available())
    ;
  String ret = Serial.readStringUntil('\r');
  Serial.println(ret);
  return ret;
}

/**
 * ポケコンからWi-Fi接続情報を設定する
 */
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
  pinMode(DIPSW, INPUT_PULLUP);
  digitalWrite(PWRLED, HIGH);
  digitalWrite(ERRLED, HIGH);

  Serial.begin(115200);
  HWSerial.begin(9600, SERIAL_8N1, GPIO_NUM_27, GPIO_NUM_14, true);

  pref.begin("G850TW");
  Serial.print(F("Connecting "));
  Serial.print(pref.getString("ssid"));
  Serial.print(F(" with password "));
  Serial.println(pref.getString("password", ""));
  // wifiMulti.addAP(pref.getString("ssid").c_str(), pref.getString("password", "").c_str());
  wifiMulti.addAP("Intel-CP-3507", "ububuntu");

  bool connOut = false;
  digitalWrite(PWRLED, connOut);
  HWSerial.flush();

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
  //SPIFFSの都合上必ずGP->kanjiの順でロードせよ
  gp.begin();
  kanji.begin();

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

    // ツイート検索
    if (command.startsWith("/"))
    {
      String text = kanji.readString();
      if (!text.isEmpty())
      {
        tw.search(now(), text.c_str(), timeline);
        currentTweet = 0;
        renderTweet(timeline);
      }
      else
      {
        gp.gprint(F("検索ワードを入力してください"));
        HWSerial.println(GP_END);
      }
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
        HWSerial.println(GP_END);
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
        HWSerial.println(GP_END);
      }
      return;
    }

    // ツイート
    if (command.startsWith("N"))
    {
      String text;
      if (digitalRead(DIPSW) == HIGH)
      {
        text = kanji.readString();
      }
      else
      {
        HWSerial.flush();
        while (!HWSerial.available())
          ;
        text = kanji.convertJapanese(kanji.htzConvert(HWSerial.readStringUntil('\r')), NULL);
        Serial.println(text);
      }
      if (!text.isEmpty())
      {
        command = text + " #ALGYAN #つくるよ #G850Tw";
        tw.post(now(), command.c_str());
      }
      HWSerial.println(GP_END);
      return;
    }

    // リプライ
    if (command.startsWith("R"))
    {
      String text = kanji.readString();
      if (!text.isEmpty())
      {
        command = text + " #ALGYAN #つくるよ #G850Tw";
        if (timeline[currentTweet].tweetId)
        {
          tw.post(now(), command.c_str(), timeline + currentTweet);
        }
        else
        {
          tw.post(now(), command.c_str());
        }
      }
      HWSerial.println(GP_END);
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
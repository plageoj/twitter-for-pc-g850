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

String inputJapanese()
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
    String input = kanji.htzConvert(HWSerial.readStringUntil('\r'));
    Serial.println(input);
    text = kanji.convertJapanese(input, NULL);
  }
  Serial.println(text);
  return text;
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
 * ネットワークを設定して再起動
 * 1行目: SSIDとする
 * 2行目: パスワードとする
 *
 */
void setNetwork()
{
  Serial.print(F("SSID: "));
  pref.putString("ssid", readLineFromSerial());
  Serial.print(F("Password: "));
  pref.putString("password", readLineFromSerial());
  ESP.restart();
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

void setHashtag()
{
  Serial.print("Hashtag: #G850Tw #");
  String tag = readLineFromSerial();
  if (tag.isEmpty())
  {
    pref.remove("hashtag");
  }
  else
  {
    pref.putString("hashtag", " #" + tag);
  }
  Serial.println("Hashtag: #G850Tw" + pref.getString("hashtag", ""));
}

void readCommand()
{
  String command;

  if (Serial.available())
  {
    command = Serial.readStringUntil('\r');
  }

  if (command.indexOf('A') != -1)
  {
    setAuthInfo();
  }
  if (command.indexOf('W') != -1)
  {
    setNetwork();
  }
  if (command.indexOf('E') != -1)
  {
    ESP.restart();
  }
  if (command.indexOf('H') != -1)
  {
    setHashtag();
  }
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

  Serial.println(F("Send command to start configuration / Auth netWork Hashtag rEset"));
  readCommand();

  Serial.print(F("Connecting to "));
  Serial.print(pref.getString("ssid", ""));
  Serial.print(F(" with password "));
  Serial.println(pref.getString("password", ""));
  wifiMulti.addAP(pref.getString("ssid").c_str(), pref.getString("password", "").c_str());

  bool connOut = false;
  int failedTimes = 0;
  digitalWrite(PWRLED, connOut);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print('.');
    readCommand();
    delay(200);
    failedTimes++;

    connOut = !connOut;
    digitalWrite(PWRLED, connOut);
    if (failedTimes == 100)
    {
      ESP.restart();
    }
  }
  digitalWrite(PWRLED, LOW);
  Serial.println(F("Connected!"));

  getTime();
  beginUdp();

  tw.begin(
      pref.getString("consumer_key").c_str(),
      pref.getString("consumer_secret").c_str(),
      pref.getString("access_token").c_str(),
      pref.getString("access_secret").c_str());
  // SPIFFSの都合上必ずGP->kanjiの順でロードせよ
  gp.begin();
  Serial.println(kanji.begin());

  digitalWrite(PWRLED, HIGH);
  digitalWrite(ERRLED, LOW);
  HWSerial.println('-'); // READY を送出
}

void loop()
{
  String command;
  static Twitter::Tweet timeline[Display_MaxData];
  static uint8_t currentTweet = 0;

  if (HWSerial.available())
  {
    command = HWSerial.readStringUntil('\r');
    Serial.println(command);
    HWSerial.readStringUntil('\n');

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
      String text = inputJapanese();
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
      String text = inputJapanese();

      if (!text.isEmpty())
      {
        command = text + pref.getString("hashtag", "") + " #G850Tw";
        tw.post(now(), command.c_str());
        gp.gprint(command);
      }
      HWSerial.println(GP_END);
      return;
    }

    // リプライ
    if (command.startsWith("R"))
    {
      String text = inputJapanese();
      if (!text.isEmpty())
      {
        command = text + pref.getString("hashtag", "") + " #G850Tw";
        if (timeline[currentTweet].tweetId)
        {
          command = "@" + timeline[currentTweet].userScreenName + " " + command;
          tw.post(now(), command.c_str(), timeline + currentTweet);
        }
        else
        {
          tw.post(now(), command.c_str());
        }
        gp.gprint(command);
      }
      HWSerial.println(GP_END);
      return;
    }

    // バッファクリアして同期
    if (command.endsWith("-"))
    {
      HWSerial.flush();
      HWSerial.println('-');
      return;
    }
  }

  readCommand();
}
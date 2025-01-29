#include <Arduino.h>
#include <WiFiMulti.h>
#include <Preferences.h>

#include <TwitterLib.h>
#include <GPRINT.h>
#include <KINPUT.h>

#define PWR_LED 25
#define ERR_LED 33
#define BUZZER 32
#define DIP_SW 34

WiFiMulti wifiMulti;

// ツイート取得件数
#define Display_MaxData 3
// 最大同時変換文節
#define Max_Segments 15

HardwareSerial HWSerial(1);
Preferences pref;
Twitter tw;
GPrint gp;
kInput kanji(Max_Segments);

/**
 * 与えられたツイートを描画する
 * @param *tweet ツイートデータ
 */
void renderTweet(Twitter::Tweet *tweet)
{
  Serial.print('@');
  Serial.println(tweet->userScreenName);
  Serial.println(tweet->tweetId);

  gp.gPrint(tweet->name);
  HWSerial.println("\xe2"); // テキストモード
  HWSerial.println(GP_NL);
  gp.gPrint(tweet->text);
  HWSerial.println(GP_END);
  HWSerial.flush();
}

String inputJapanese()
{
  String text;
  if (digitalRead(DIP_SW) == HIGH)
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
  String ret = "";
  while (true)
  {
    while (!Serial.available())
      ;
    char c = Serial.read();
    if (c == '\r')
    {
      Serial.read(); // LFを読み飛ばす
      Serial.println();
      break;
    }
    Serial.print(c);
    ret += c;
  }
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
  String ssid = readLineFromSerial();
  Serial.print(F("Password: "));
  String password = readLineFromSerial();
  if (ssid.isEmpty() || password.isEmpty())
  {
    Serial.println(F("Cancelled setting network up"));
    return;
  }
  pref.putString("ssid", ssid);
  pref.putString("password", password);
  ESP.restart();
}

void setAuthInfo()
{
  Serial.print(F("RapidApi key: "));
  pref.putString("rapid_api_key", readLineFromSerial());

  Serial.print(F("Twttr Session: "));
  pref.putString("twttr_session", readLineFromSerial());

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
  char command;

  if (Serial.available())
  {
    command = Serial.read();
  }

  if (command == 'A')
  {
    setAuthInfo();
  }
  if (command == 'W')
  {
    setNetwork();
  }
  if (command == 'E')
  {
    ESP.restart();
  }
  if (command == 'H')
  {
    setHashtag();
  }
}

void setup()
{
  pinMode(PWR_LED, OUTPUT);
  pinMode(ERR_LED, OUTPUT);
  pinMode(DIP_SW, INPUT_PULLUP);
  digitalWrite(PWR_LED, HIGH);
  digitalWrite(ERR_LED, HIGH);

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
  digitalWrite(PWR_LED, connOut);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print('.');
    readCommand();
    delay(200);
    failedTimes++;

    connOut = !connOut;
    digitalWrite(PWR_LED, connOut);
    if (failedTimes == 100)
    {
      ESP.restart();
    }
  }
  digitalWrite(PWR_LED, LOW);
  Serial.println(F("Connected!"));

  tw.begin(pref.getString("rapid_api_key").c_str(),
           pref.getString("twttr_session").c_str());
  // SPIFFSの都合上必ずGP->kanjiの順でロードせよ
  if (gp.begin())
  {
    Serial.println("GPrint loaded");
  }
  Serial.println(kanji.begin());

  digitalWrite(PWR_LED, HIGH);
  digitalWrite(ERR_LED, LOW);
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
      tw.getUserTimeline(command.c_str(), timeline);
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
        tw.search(text.c_str(), timeline);
        currentTweet = 0;
        renderTweet(timeline);
      }
      else
      {
        gp.gPrint(F("検索ワードを入力してください"));
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
        gp.gPrint(F("取得件数外です"));
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
        gp.gPrint(F("これ以前のツイートはありません"));
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
        tw.post(command.c_str());
        gp.gPrint(command);
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
          tw.post(command.c_str(), timeline + currentTweet);
        }
        else
        {
          tw.post(command.c_str());
        }
        gp.gPrint(command);
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
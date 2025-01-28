#include <Arduino.h>
#include <ESP32_SPIFFS_MisakiFNT.h>
#include <SPIFFS.h>
#include "GPRINT.h"

ESP32_SPIFFS_MisakiFNT misaki;
extern HardwareSerial HWSerial;
File zth;

#define TEXT_MODE 0
#define HIRAGANA_MODE 1
#define KANJI_MODE 2

bool GPrint::begin(const char *utf2JisTable, const char *hanFont, const char *zenFont, const char *z2hTable)
{
    misaki.SPIFFS_Misaki_Init3F(utf2JisTable, hanFont, zenFont);
    zth = SPIFFS.open(z2hTable, "r");
    return zth;
}

void GPrint::flowControl()
{
    if (HWSerial.read() == 19)
    {
#ifdef DEBUG
        Serial.println("XOFF");
#endif
        while (HWSerial.read() != 17)
            delay(10);
#ifdef DEBUG
        Serial.println("XON");
#endif
    }
    delay(2);
}

void GPrint::clearBuffer()
{
    HWSerial.println();
#ifdef DEBUG
    Serial.print("*");
#endif
    HWSerial.readStringUntil('-');
}

void GPrint::gPrint(String utf8)
{
    uint8_t gpBuf[280][8] = {};
    int prevMode = TEXT_MODE;
    utf8.replace("\n", "");
    const char *cpt = utf8.c_str();
    uint8_t outlen;
#ifdef DEBUG
    Serial.println(utf8);
#endif
    while (*cpt)
    {
        // マルチバイト文字を検出する
        outlen = 0;
        while (*cpt > 0x7F)
        {
            // ひらがな・カタカナを検出
            if (*cpt == 0xe3 && cpt[1] >= 0x81 && cpt[1] <= 0x83)
            {
                break;
            }
            cpt++;
            outlen++;
        }
        // マルチバイト文字を書き出す
        if (outlen)
        {
            String utf = String(cpt - outlen).substring(0, outlen);
            uint16_t len = misaki.StrDirect_MisakiFNT_readALL(utf, gpBuf) / 2;
            uint8_t spaces = 0;
            flowControl();
            if (prevMode != KANJI_MODE)
            {
                HWSerial.println("\n\xe3");
                prevMode = KANJI_MODE;
            }
            for (int i = 0; i < len; i++)
            {
                outlen = 0;
                for (int j = 0; j < 8; j++)
                {
                    uint8_t rotBuf = 0;
                    // フォントデータは90度回転した行列で来るので、読める向きに回す
                    for (int k = 0; k < 8; k++)
                        rotBuf += (gpBuf[i][k] & 1 << (7 - j)) ? 1 << k : 0;

                    if (rotBuf == 0 || rotBuf == 255)
                        spaces++;
                    else
                        spaces = 0;

                    // 空白が連続したら出力しない
                    if (spaces <= 7)
                    {
                        HWSerial.print(rotBuf >> 4, HEX);
                        HWSerial.print(rotBuf & 15, HEX);

#ifdef DEBUG
                        Serial.print(rotBuf >> 4, HEX);
                        Serial.print(rotBuf & 15, HEX);
#endif
                        outlen++;
                    }
                }
                // 母艦バッファをクリアするために改行を送出
                if (outlen)
                {
#ifdef DEBUG
                    Serial.println();
#endif
                    HWSerial.println();
                    flowControl();
                    if (i % 4 == 3)
                    {
                        clearBuffer();
                    }
                }
            }
        }

        // ひらがな・カタカナを書き出す
        outlen = 0;
        while (*cpt == 0xe3)
        {
            uint32_t addr = (cpt[1] - 0x81) * 0x40 + cpt[2] - 0x80;

            bool hk = zth.seek(addr);
            if (!hk)
            { // テーブルに文字がない
#ifdef DEBUG
                Serial.print(F("T- "));
#endif
                cpt += 3;
                continue;
            }
            if (addr > 0x60 && prevMode != TEXT_MODE) // カタカナ
            {
                HWSerial.println("\n\xe2"); // テキストモード
                prevMode = TEXT_MODE;
#ifdef DEBUG
                Serial.print(F("\nカタカナ: "));
#endif
            }
            else if (addr < 0x60 && prevMode != HIRAGANA_MODE) // ひらがな
            {
                HWSerial.println("\n\xe1"); // ひらがなモード
                prevMode = HIRAGANA_MODE;
#ifdef DEBUG
                Serial.print(F("\nひらがな: "));
#endif
            }
#ifdef DEBUG
            Serial.print(addr, HEX);
            Serial.print(F(" "));
#endif
            char letter = zth.read();
            flowControl();
            switch (letter)
            {
            case 0xdf: // 半濁点
                addr--;
            case 0xde: // 濁点
                zth.seek(addr - 1);
                HWSerial.print((char)zth.read());
                HWSerial.print(letter);
                outlen += 2;
                break;
            case 0:
                break;
            default:
                HWSerial.print(letter);
                outlen++;
            }
#ifdef DEBUG
            Serial.print(cpt[0]);
            Serial.print(cpt[1]);
            Serial.print(cpt[2]);
#endif
            cpt += 3;

            if (outlen > 22)
            {
                outlen = 0;
                clearBuffer();
            }
            flowControl();
        }
        if (outlen)
            clearBuffer();

        // シングルバイト文字を書き出す
        if (*cpt && *cpt < 0x80)
        {
            if (prevMode != TEXT_MODE)
            {
                HWSerial.println("\n\xe2");
                prevMode = TEXT_MODE;
#ifdef DEBUG
                Serial.print(F("\nテキスト: "));
#endif
            }
            cpt += putSingleByteChar(cpt);
#ifdef DEBUG
            Serial.println();
#endif
        }
    }

    HWSerial.flush();
    return;
}

int GPrint::putSingleByteChar(const char *str)
{
    int outlen = 0;
    while (*str && *str < 0x80)
    {
        flowControl();
        HWSerial.write(*str);
        str++;
        outlen++;
#ifdef DEBUG
        Serial.print(*str);
#endif
        if (outlen % 150 == 0)
        {
            clearBuffer();
        }
    }
    if (outlen % 150)
        clearBuffer();
    flowControl();

    return outlen;
}

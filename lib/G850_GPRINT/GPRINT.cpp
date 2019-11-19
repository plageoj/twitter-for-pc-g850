#include <Arduino.h>
#include <ESP32_SPIFFS_MisakiFNT.h>
#include <SPIFFS.h>
#include "GPRINT.h"

ESP32_SPIFFS_MisakiFNT misaki;
extern HardwareSerial HWSerial;
File zth, htz;

#define TEXT_MODE 0
#define HIRA_MODE 1
#define KNJI_MODE 2

bool GPRINT::begin(const char *ujistable, const char *hanfont, const char *zenfont, const char *zthtable, const char *htztable)
{
    bool misakiresult = misaki.SPIFFS_Misaki_Init3F(ujistable, hanfont, zenfont);

    zth = SPIFFS.open(zthtable, "r");
    htz = SPIFFS.open(htztable, "r");

    return zth && htz && misakiresult;
}

void GPRINT::flowControl()
{
    if (HWSerial.read() == 19)
    {
#ifdef DEBUG
        Serial.println("XOFF");
#endif
        while (HWSerial.read() != 17)
            delay(20);
#ifdef DEBUG
        Serial.println("XON");
#endif
    }
    delay(10);
}

void GPRINT::clearBuffer()
{
    HWSerial.println();
    HWSerial.readStringUntil('-');
}

void GPRINT::gprint(String utf8)
{
    uint8_t gpbuf[280][8] = {};
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
            uint16_t len = misaki.StrDirect_MisakiFNT_readALL(utf, gpbuf) / 2;
            uint8_t spaces = 0;
            flowControl();
            if (prevMode != KNJI_MODE)
            {
                HWSerial.println("\n\xe3");
                prevMode = KNJI_MODE;
            }
            for (int i = 0; i < len; i++)
            {
                outlen = 0;
                for (int j = 0; j < 8; j++)
                {
                    uint8_t rtbuf = 0;
                    // フォントデータは90度回転した行列で来るので、読める向きに回す
                    for (int k = 0; k < 8; k++)
                        rtbuf += (gpbuf[i][k] & 1 << (7 - j)) ? 1 << k : 0;

                    if (rtbuf == 0 || rtbuf == 255)
                        spaces++;
                    else
                        spaces = 0;

                    // 空白が連続したら出力しない
                    if (spaces <= 7)
                    {
                        HWSerial.print(rtbuf >> 4, HEX);
                        HWSerial.print(rtbuf & 15, HEX);

#ifdef DEBUG
                        Serial.print(rtbuf >> 4, HEX);
                        Serial.print(rtbuf & 15, HEX);
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
                }
            }
        }

        // ひらがな・カタカナを書き出す
        outlen = 0;
        while (*cpt == 0xe3)
        {
            uint32_t addr = (cpt[1] - 0x81) * 0x40 + cpt[2] - 0x80;

            bool hk = zth.seek(addr);
            if (!hk) // テーブルに文字がない
                break;
            if (addr > 0x60 && prevMode != TEXT_MODE) //カタカナ
            {
                HWSerial.println("\n\xe2"); // テキストモード
                prevMode = TEXT_MODE;
            }
            else if (addr < 0x60 && prevMode != HIRA_MODE) // ひらがな
            {
                HWSerial.println("\n\xe1"); // ひらがなモード
                prevMode = HIRA_MODE;
            }
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
            }
            cpt += putSingleByteChar(cpt);
        }
    }

    HWSerial.flush();
    return;
}

int GPRINT::putSingleByteChar(const char *str)
{
    int outlen = 0;
    while (*str && *str < 0x80)
    {
        flowControl();
        HWSerial.write(*str);
        str++;
        outlen++;
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

String GPRINT::htzConvert(String text)
{
    bool hiramode = true;
    unsigned int pos = 0, len = text.length();
    char buf[4] = "";
    String ret = "";

    while (len - pos)
    {
        char r = text.charAt(pos);
        if (r == 248) // 秒記号 ′′
        {
            hiramode = !hiramode;
        }
        else if (r >= 0xa1 && r <= 0xdf) // 仮名
        {
            uint32_t adr = (r - 0xa1) * 3 + (hiramode ? 0xc1 : 1);
            htz.seek(adr);
            htz.readBytes(buf, 3);

            switch (text.charAt(pos + 1)) // 次の文字が
            {
            case 0xdf: // 半濁点
                buf[2]++;
            case 0xde: // 濁点
                buf[2]++;
                pos++;
            }
            ret.concat(buf);
        }
        else
        {
            ret.concat(r);
        }
        pos++;
    }

#ifdef DEBUG
    Serial.println();
    Serial.println(ret);
#endif
    return ret;
}
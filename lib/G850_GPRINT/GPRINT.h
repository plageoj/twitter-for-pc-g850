#ifndef G850_GPRINT_H_INCLUDED
#define G850_GPRINT_H_INCLUDED

#include <Arduino.h>
#include <SPIFFS.h>
#include <ESP32_SPIFFS_MisakiFNT.h>

#define GP_NL '\xf0'
#define GP_END '\xee'

class GPRINT
{
public:
    /**
 * ポケコン漢字ライブラリの利用を開始する
 *
 * @param ujistable UTF8→SJIS変換テーブルのパス
 * @param hanfont 半角フォントのパス
 * @param zenfont 全角フォントのパス
 * @param zthtable 全角→半角変換テーブルのパス
 * @param htztable 半角→全角変換テーブルのパス
 *
 * @returns ファイルが開ければ true
 */
    bool begin(const char *ujistable, const char *hanfont, const char *zenfont, const char *zthtable);
    bool begin(const char *ujistable, const char *hanfont, const char *zenfont)
    {
        return begin(ujistable, hanfont, zenfont, "/zth.tbl");
    }
    bool begin()
    {
        return begin("/Utf8Sjis.tbl", "/4X8.FNT", "/MSKG13KU.FNT");
    }
    /**
 * UTF-8 で与えられた文字列をレンダリングし
 * HEXとしてシリアルに送出する
 *
 * @param text 変換する文字列
 */
    void gprint(String text);
    void gprint(char *text)
    {
        return gprint(String(text));
    }

private:
    /**
 *  シリアル通信のソフトウェアフロー制御
 * Serial.print 中でときどき呼ぶ
 * XOFFがあるまで関数を return しない
 */
    void flowControl();
    /**
     * ポケコン側のバッファクリアを待つ
     */
    void clearBuffer();
    /**
     * シングルバイト文字をダイレクトモードで書き出す
     * @returns 出力した文字数
     */
    int putSingleByteChar(const char *);
};

#endif
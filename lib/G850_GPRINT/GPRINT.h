#ifndef G850_G_PRINT_H_INCLUDED
#define G850_G_PRINT_H_INCLUDED

#include <Arduino.h>
#include <SPIFFS.h>
#include <ESP32_SPIFFS_MisakiFNT.h>

#define GP_NL '\xf0'
#define GP_END '\xee'

class GPrint
{
public:
    /**
     * ポケコン漢字ライブラリの利用を開始する
     *
     * @param utf2JisTable UTF8→SJIS変換テーブルのパス
     * @param hanFont 半角フォントのパス
     * @param zenFont 全角フォントのパス
     * @param z2hTable 全角→半角変換テーブルのパス
     *
     * @returns ファイルが開ければ true
     */
    bool begin(const char *utf2JisTable, const char *hanFont, const char *zenFont, const char *z2hTable);
    bool begin(const char *utf2JisTable, const char *hanFont, const char *zenFont)
    {
        return begin(utf2JisTable, hanFont, zenFont, "/zth.tbl");
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
    void gPrint(String text);
    void gPrint(char *text)
    {
        return gPrint(String(text));
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
#include <Arduino.h>
#include <SPIFFS.h>
#include <ESP32_SPIFFS_MisakiFNT.h>

#ifndef G850_GPRINT_H_INCLUDED
#define G850_GPRINT_H_INCLUDED

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
    bool begin(const char *ujistable, const char *hanfont, const char *zenfont, const char *zthtable, const char *htztable);
    bool begin(const char *ujistable, const char *hanfont, const char *zenfont)
    {
        return begin(ujistable, hanfont, zenfont, "/zth.tbl", "/htz.tbl");
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
 * @returns 出力した文字数
 */
    int gprint(String text);
    int gprint(char *text)
    {
        return gprint(String(text));
    }

    /**
     * SJISの半角カナをUTF-8の全角カナに変換する
     * ひらがな/カタカナモードは長音で切り替え
     * デフォルトはひらがな
     * @param text 半角カナまじり文
     * @returns 全角カナまじり文
     */
    String htzConvert(String text);

private:
    /**
 *  シリアル通信のソフトウェアフロー制御
 * Serial.print 中でときどき呼ぶ
 * XOFFがあるまで関数を return しない
 */
    void flowControl();
    /**
     * シングルバイト文字をダイレクトモードで書き出す
     * @returns 出力した文字数
     */
    int putSingleByteChar(const char *);
};

#endif
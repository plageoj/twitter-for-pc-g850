#ifndef G850_KINPUT_H_INCLUDED
#define G850_KINPUT_H_INCLUDED

#include <Arduino.h>
#include <WiFiClient.h>

/**
 * ポケコン用シリアル連文節変換入力 KINPUT
 */
class KINPUT
{
public:
    struct ConversionResult
    {
        String yomi;
        String candidates[5] = {};
    };
    /**
     * 連文節変換の設定
     * @param max_segs 最大分節数
     */
    KINPUT(unsigned int max_segs);
    /**
     * 連文節変換エンジンへ接続する
     * @param htztable 半角→全角テーブルのファイル名
     * @returns 接続成功したら true
     */
    int begin(const char *htztable);
    /**
     * 連文節変換エンジンへ接続する
     * @returns 接続成功したら true
     */
    int begin()
    {
        return begin("/htz.tbl");
    }
    /**
     * 与えられたひらがな文字列を連文節変換する
     * @param hiragana ひらがな文字列 UTF-8
     * @param resultSet 結果セットの配列へのポインタ
     * @returns 最もそれらしい結果
 */
    String convertJapanese(String hiragana, ConversionResult *);
    /**
     * 連文節変換・候補選択を行う
     * @returns 変換結果
     */
    String readString();
    /**
     * SJISの半角カナをUTF-8の全角カナに変換する
     * ひらがな/カタカナモードは長音で切り替え
     * デフォルトはひらがな
     * @param text 半角カナまじり文
     * @returns 全角カナまじり文
     */
    String htzConvert(String text);
};
#endif
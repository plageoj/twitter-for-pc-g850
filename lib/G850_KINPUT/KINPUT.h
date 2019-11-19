#include <Arduino.h>

#ifndef G850_KINPUT_H_INCLUDED
#define G850_KINPUT_H_INCLUDED

struct ConversionResult
{
    String yomi;
    String candidates[5] = {};
};
/**
     * 与えられたひらがな文字列を連文節変換する
     * @param hiragana ひらがな文字列 UTF-8
     * @param resultSet 結果セットの配列へのポインタ
     * @returns 最もそれらしい結果
 */
String convertJapanese(String hiragana, ConversionResult *);

#endif
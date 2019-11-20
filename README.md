# Twitter for PC-G850

ポケットコンピュータ PC-G850 シリーズ向け Twitter クライアント用拡張カートリッジ

## カートリッジ回路について

ESP32 DevkitC を使用しています。バージョンは問いません。
本ソースでは最低限以下の接続が必要となります。

| ESP32 | ポケコン |
|:-----:|:-------:|
| GPIO27| RXD     |
| GPIO14| TXD     |
| GND   | GND     |

G850シリーズの11ピン端子に接続し、SIOモードで通信します。
詳しくは[マニュアル](https://www.manualslib.com/manual/1251415/Sharp-Pc-G850vs.html?page=3#manual)を参照してください。

なお、シリアル通信に15ピン端子を使用する機種では、ピンピッチ変換が必要になります。接続方法は取扱説明書を参照してください。

## ポケコン側の設定

ポケコンでは以下のとおり設定してください。

```
Baud rate   = 9600
Data bit    = 8
Stop bit    = 1
Parity      = none
End of line = CRLF
Flow control= Xon/Xoff
```

本体にはこちらの[BASICプログラム](https://gist.github.com/plageoj/075a75bcc3708cc6b8f04bbf33c79d75)をロードしておく必要があります。

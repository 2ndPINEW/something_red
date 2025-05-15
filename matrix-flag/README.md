| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# このプロジェクトは何？

esp-idfの Hello Worldベースに作った、LEDピカピカ光らせるプロジェクト。
FreeRTOSでタスクが動作します。

# ディレクトリ構成

- `/components` 外部ライブラリ
  - 使っているライブラリを git submoduleで埋め込んでいます
- `/main` プロジェクトルート
  - main.c がエントリポイントです

# 開発準備
- [esp-idf](https://github.com/espressif/esp-idf)
- esp32s3を使ってます
- 
![alt text](assets/755a3b84-f4b8-41fe-8aec-e1054749151e.png)

# データ形式
SSEのデータストリームをパクった形式にする

カラーパレットの最大数は 256

```markdown
各データはHEXカラーコードをカンマ区切りで送る

palette: #ffffff, #000000
flame: 時間, 0x0の色index, 0x1の色index ...... 
flame: 時間, 0x0の色index, 0x1の色index ...... 
flame: 時間, 0x0の色index, 0x1の色index ...... 
flame: 時間, 0x0の色index, 0x1の色index ...... 
END
```

# Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

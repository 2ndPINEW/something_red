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


# Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

# Midi2Switch

**MIDI キーボード → Nintendo Switch** を実現するためのデュアル ATOM S3 Lite ガジェット用 Arduino スケッチです。
MIDI キーボードのノート ON/OFF を Nintendo Switch の USB コントローラー入力へ変換します。

---

## 特長

|             | BLE 接続版                         | UART 接続版                                 |
| ----------- | ------------------------------- | ---------------------------------------- |
| 通信経路        | ATOMS3\_A → **BLE** → ATOMS3\_B | ATOMS3\_A → **UART (GROVE)** → ATOMS3\_B |
| ケーブル不要      | Yes                             | No (GROVE)                               |
| レイテンシ       | 数 ms（測定値 < 5 ms）                | 有線なのでさらに低遅延                              |
| ファーム同梱スケッチ数 | 2（A/B）                          | 2（A/B）                                   |

* Nintendo Switch には **USB コントローラー** として接続
* ATOM S3 Lite ×2 という **安価かつ入手しやすい** 構成。
* LED で接続状態を即座に確認。
* ATOMS3\_B の物理ボタン（短押し）で **マッピング 1 / 2** を即時切替  
* ATOMS3\_B の物理ボタン（長押し）で **RS ボタン**を送信（ゲーム内のキー配置を変更）  
* D-Pad（十字キー）の斜め／同時入力には対応していますが、上下／左右といった逆方向の同時入力には対応していません。  
* アナログスティックは斜めも含めて同時入力には対応していません。  

---

## ハードウェア構成

| 必要なもの                | 用途                                     |
| -------------------- | -------------------------------------- |
| ATOM S3 Lite × **2** | A: MIDI 受信 / B: Switch に USB デバイスとして接続 |
| USB‑C OTG ケーブル       | ATOMS3\_B ⇒ Nintendo Switch 接続         |
| MIDI キーボード (USB)     | ATOMS3\_A に接続                          |
| GROVE ケーブル           | （UART 版のみ）A ↔ B 接続                     |
| 5V 電源       | ATOMS3\_A を給電                         |

> **注意**  
> GROVE ケーブルを使用する場合は5V ライン（赤い線）を必ずカットすること   
> （ケーブルを物理的に切断するか、コネクタから外してください）   
> 5V電源同士が衝突した場合、破損・発煙の可能性があります。

> **なぜ 2 台構成?**
> * Switch は BLE 非対応 (Classic のみ)／ATOM S3 は Bluetooth Classic 非対応
> * ATOM S3 Lite は USB‑C が 1 口しかない – USB Host と Device を同時に扱えない
> * USB ハブを S3 で制御するのはハードルが高い
>   → **USB Device 専用 (B) / USB Host 専用 (A) に分離** するのがシンプルで確実。

---

## LED インジケータ (ATOMS3\_B, BLE 版)

| 色     | 状態            |
| ----- | ------------- |
| **白** | 電源投入直後（待機）    |
| **緑** | BLE スキャン開始    |
| **青** | 接続成功          |
| **赤** | 接続失敗 / タイムアウト |

* **赤**になったら → ATOMS3\_A の電源を確認し、ATOMS3\_B の RESET を押す。
* **白**のまま長時間変化しない → ATOMS3\_A の電源を確認し、ATOMS3\_A を RESET。

---

## ソフトウェア要件

* Arduino IDE 2.3 以降
* Espressif **esp32**   ボードパッケージ 3.2 以降  
* M5Stack   **M5Stack** ボードパッケージ 3.2 以降  
* ライブラリ  
  | ライブラリ | バージョン | ライセンス |
  | ---------- | ---------- | ---------- |
  | [M5AtomS3](https://github.com/m5stack/M5AtomS3) | 1.0.2 | MIT |
  | [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | 2.3.0 | Apache-2.0 |
  | [ESP32_Host_MIDI](https://github.com/sauloverissimo/ESP32_Host_MIDI) | 1.0.3 | MIT |
  | [switch_ESP32](https://github.com/esp32beans/switch_ESP32) | - | MIT |

・switch_ESP32 はリポジトリから ZIP をダウンロードして   
  Arduino IDE の「Sketch> Include Library> Add .ZIP Library」でライブラリにインストールしてください。   
・ESP32_Host_MIDI は`USB_Conexion`のみ使用しているのでスケッチフォルダに同梱しています。   
  ※ライブラリ全体をインストールするとBLE 関連（NimBLE）が二重定義となりビルドできなくなります。   
・上記以外のライブラリはライブラリマネージャからインストール可能です。   

---

## ビルド & 書き込み手順

1. このリポジトリをクローン / ZIP ダウンロードして展開  
2. BLE版、UART版どちらかの`ATOMS3_A_Midi.ino` or `ATOMS3_B_Switch`を Arduino IDE で開く  
3. 上記依存ライブラリをインストール  
4. 「File> Preferences> Additional boards manager URLs:」に以下のURLを追加する   
   https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
5. 「tools> Board> M5Stack> M5AtomS3」を選択  
6. ポートを選択（PC 環境によりポート番号は変わります）
7. 「USB CDC On Boot: **"Disabled"**」を選択
8. 「USB Mode: **"USB-OTG (TinyUSB)"**」を選択
9. **スケッチを書き込み**  

   | フォルダ                          | 用途                   | 書き込む ATOM |
   | ----------------------------- | -------------------- | --------- |
   | `src_ble/ATOMS3_A_BLE`   | BLE 送信側              | ATOMS3\_A |
   | `src_ble/ATOMS3_B_BLE`   | BLE 受信 & Switch USB  | ATOMS3\_B |
   | `src_uart/ATOMS3_A_UART` | UART 送信側             | ATOMS3\_A |
   | `src_uart/ATOMS3_B_UART` | UART 受信 & Switch USB | ATOMS3\_B |

> **注意**  
> USB-OTG モードで書き込みした後は、そのままでは書き込みできなくなります。   
> ATOMS3 Lite のリセットボタンを 2 秒以上押すと LED が緑色に一瞬だけ光り書き込みできるようになります。   

---

## 使い方

### BLE 版

1. ATOMS3\_B を Switch と USB‑C で接続し、電源投入 (白 LED)。
2. ATOMS3\_A に MIDI キーボードを接続し、電源投入。
3. ATOMS3\_B が **緑 → 青** になれば接続完了。
4. キーボードを弾くと Switch でボタン入力として認識されます。

### UART 版

1. GROVE ケーブルで A ↔ B を接続。
2. ATOMS3\_B を Switch と USB‑C で接続。
3. 起動後すぐに入力が伝送されます

---

## ライセンス

* **Midi2XInput (本コード)** — MIT  
* **USB_Conexion (ESP32_Host_MIDI 部分)** — MIT   
* そのほか M5AtomS3 / NimBLE-Arduino (Apache-2.0) / switch_ESP32 (MIT) については各リポジトリを参照。  
  まとめは [`THIRD_PARTY_LICENSES.md`](./THIRD_PARTY_LICENSES.md) に記載。

## 免責事項
『Nintendo Switch』に関して   
本プロジェクトは Nintendo および 任天堂株式会社 とは一切関係ありません。   

『Sky 星を紡ぐ子どもたち』に関して   
本プロジェクトは thatgamecompany, Inc. ならびに同作のパブリッシャー・配信プラットフォームと一切関係ありません。

本プロジェクトを使用した結果生じたいかなる損害（アカウント停止・データ消失・その他トラブル）についても、作者は責任を負いかねます。こちらも 自己責任 でご利用ください。

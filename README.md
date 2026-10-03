# EWI MIDI Message Tester / Emulator

EWI（ウインドシンセサイザー）特有のMIDI制御メッセージ（Note, Pitch Bend, Breath Control, Expression, Modulation等）を擬似出力するLV2プラグインです。ソフト音源やエフェクトのブレスレスポンス評価やマッピングテスト等に使用できます。

## 特徴

* **ノートトリガー機能**: Enable を ON にすると Note On、OFF にすると Note Off を発行します。
* **スラー対応**: 演奏中に Note Number を変更すると、直前のノートの Note Off と新しいノートの Note On を連続発行します。
* **高頻度CC/PB送出**: 設定したミリ秒間隔（デフォルト 20ms）で Pitch Bend および各種 CC（Breath, Expression, Modulation, Extra CC）を継続送信します。

## ビルドとインストール

### 依存関係
* `lv2-dev`
* `pkg-config`
* `build-essential` (gcc, make)

### 手順

```bash
# ビルド
make

# インストール（~/.lv2/ に配置されます）
make install

```

## パラメータ仕様

| ポート名 | 内部シンボル | デフォルト | 範囲 | 説明 |
| --- | --- | --- | --- | --- |
| **Enable** | `enable` | 0 (Off) | 0 - 1 | 送信制御（1でNote On発行、0でNote Off発行） |
| **Interval (ms)** | `interval_ms` | 20 | 1 - 2000 | CC/PBメッセージを送出する更新間隔 |
| **Channel** | `channel` | 0 | 0 - 15 | 送信MIDIチャンネル（Ch 1 - 16） |
| **Note Number** | `note_number` | 60 | 0 - 127 | ノート番号（C4 = 60） |
| **Velocity** | `velocity` | 64 | 1 - 127 | ノートオン時のベロシティ |
| **Pitch Bend** | `pb_value` | 0 | -8192 - 8191 | ピッチベンド値 |
| **Breath (CC#2)** | `cc_breath` | 100 | 0 - 127 | ブレスコントロール（CC #2） |
| **Expression (CC#11)** | `cc_expression` | 0 | 0 - 127 | エクスプレッション（CC #11） |
| **Modulation (CC#1)** | `cc_modulation` | 0 | 0 - 127 | モジュレーション（CC #1） |
| **Extra CC Num** | `cc_extra_num` | 7 | 0 - 127 | 任意割り当て用CC番号 |
| **Extra CC Val** | `cc_extra_val` | 0 | 0 - 127 | 任意割り当て用CC値 |


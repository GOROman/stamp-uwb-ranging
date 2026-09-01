# stamp-uwb-ranging

M5Stamp C5 + Stamp UWB F で DS-TWR 測距するプロトタイプ。

調査日: 2026-09-01。公式ドキュメントと [M5Stamp-UWB](https://github.com/m5stack/M5Stamp-UWB) に基づく。

## スケッチ

| スケッチ | 役割 | パス |
|---|---|---|
| DS_TWR_TAG | タグ（イニシエーター） | `sketches/DS_TWR_TAG/` |
| DS_TWR_ANCHOR | アンカー（レスポンダー） | `sketches/DS_TWR_ANCHOR/` |

スケッチは `include/` のヘッダ（`uwb_pins.h`, `uwb_addrs.h`, `uwb_phy.h`）を参照する。Arduino IDE で開く場合は [スケッチパスの設定](docs/sketch_paths.md) を参照。

## CI（GitHub Actions）

push / pull_request 時に全スケッチを M5StampC5 向けにコンパイル。ハードウェアへのフラッシュは行わない（コンパイルのみ）。フラッシュは PC から USB 経由で Stamp-C5 に直接行う（外部プログラマー不要）。

- FQBN: `m5stack:esp32:m5stack_stamp_c5`
- ボードインデックス: https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json

## ハードウェア（手元）

| 品 | 数量 | 備考 |
|---|---|---|
| Stamp UWB F（S017-F） | 5 | QM33120W、FPC 0.5mm-12P。この測距では 3 台をホストする |
| Stamp UWB（S017） | 2 | SMT。今回は未使用 |
| Stamp-C5 ノーマル | 6 | 公式 3 + AliExpress 3。この測距では手元 6 のうち 3 をホストに使う。DIP ではない |

S017 と S017-F はチップ・RF 仕様は同じ。差はコネクタだけ。公式クレームは正対 55 m、DS-TWR 誤差約 0.14 m（アンテナ遅延は未校正。ライブラリ既定 16385）。

## ピン（UWB-F → Stamp-C5）

公式サンプルとライブラリ既定と一致。直さなくていい。

| UWB-F | C5 GPIO |
|---|---|
| GP7 | G23 |
| IRQ | G0 |
| WAKEUP | G24 |
| RST | G25 |
| MISO | G26 |
| MOSI | G27 |
| CS | G11 |
| SCK | G12 |

詳細は [docs/wiring.md](docs/wiring.md)。

## 初回フラッシュ（2台）

1. C5×2 に UWB-F を FPC で接続。向き確認。アンテナにケーブルを重ねない。
2. Arduino IDE: ボード `M5StampC5`、ライブラリ `M5Stamp_UWB`。
3. 先に `sketches/DS_TWR_ANCHOR/` を Anchor 用 C5 に書き込み、次に `sketches/DS_TWR_TAG/` を Tag 用 C5 に書き込む。
   - Arduino IDE でスケッチを開く前に [スケッチパスの設定](docs/sketch_paths.md) を参照。
4. PHY はライブラリ既定 **Channel 9**。Arduino ドキュメントの Channel 5 直書き例は使わない。混ぜると無通信。
5. シリアル 115200。`UWB_ID,dev_id=0xDECA0314` と `DS_RANGE_STAT,...distance_mm=` が出れば成功。

PAN は両端 `0xDECA`。Anchor `0x0002`、Tag `0x0001`。

**フラッシュ方法**: PC と Stamp-C5 を USB Type-C で接続し、Arduino IDE から直接書き込む。外部プログラマーは不要。

## 3台目

Anchor `0x0003` を足す（ドキュメント §5 形式）。GitHub の MULTI 例は `0x0101` 系で別物。混ぜない。

公式ライブラリは順次ペアワイズ DS-TWR のみ。TDoA / 2D・3D 測位は未実装。座標は上位で計算する。

## やってはいけないこと

- Stamp-S3 / S3A の背面 FPC へ直接つなぐ（ピン非互換、永久破損）
- FPC 逆差し
- FPC を PCB アンテナ下に通す
- ドキュメントの Ch5 例と GitHub の Ch9 例を混在させる

詳細は [docs/gotchas.md](docs/gotchas.md)。シリアル形式は [docs/serial_format.md](docs/serial_format.md)。

## リンク

- [Arduino チュートリアル](https://docs.m5stack.com/ja/arduino/projects/stamp/stamp_uwb)
- [Stamp UWB F](https://docs.m5stack.com/ja/stamp/Stamp_UWB_F)
- [Stamp UWB](https://docs.m5stack.com/ja/stamp/Stamp_UWB)
- [Stamp-C5](https://docs.m5stack.com/en/core/Stamp-C5)
- [ライブラリ M5Stamp-UWB](https://github.com/m5stack/M5Stamp-UWB)

# 配線

Stamp UWB F（S017-F）を Stamp-C5 ノーマルの背面 0.5mm-12P に FPC ケーブルでつなぐ。C5 DIP は未所持。電気的ピンは DIP と同一。

## UWB-F 0.5mm-12P（モジュール側）

出典: [製品ページ ピンマップ](https://docs.m5stack.com/ja/stamp/Stamp_UWB_F)。裏面、コネクタを下にしたとき **右が pin 1、左が pin 12**（図の `12 ← 1`）。

| Pin | 信号 | 色（公式図） |
|---|---|---|
| 1 | 3V3 | POWER |
| 2 | 3V3 | POWER |
| 3 | GP7 | GPIO |
| 4 | IRQ | GPIO |
| 5 | WAKEUP | POWER/CTRL |
| 6 | RSTn | POWER/CTRL |
| 7 | GND | GND |
| 8 | MISO | SPI（チップ DW_CDO） |
| 9 | MOSI | SPI（チップ DW_CDI） |
| 10 | CSn | SPI |
| 11 | GND | GND |
| 12 | CLK | SPI |

回路図 `SCH_UWB_MODULE` の J1 も pin3=GP7, 4=IRQ, 5=WAKEUP, 6=RSTn, 8=CDO, 9=CDI, 10=CSn, 12=CLK。pin1/2 は製品ピンマップどおり **両方 3V3**（C5 側も 1–2 が 3V3）。図下右 PINMAP 箱は SMT キャステレーション用で、フレキではない。

手元ケーブルは `0.5*12P*50MM 同面`。同面なら C5 背面 FPC と **同番号 1:1**。

## 信号（UWB-F → Stamp-C5 GPIO）

| UWB-F | C5 GPIO | ライブラリ `M5Stamp_UWBConfig` |
|---|---|---|
| GP7 | G23 | `pin_gp7 = 23` |
| IRQ | G0 | `pin_irq = 0` |
| WAKEUP | G24 | `pin_wakeup = 24` |
| RST | G25 | `pin_rst = 25` |
| MISO | G26 | `pin_miso = 26`（チップ DW_CDO → host MISO） |
| MOSI | G27 | `pin_mosi = 27`（チップ DW_CDI → host MOSI） |
| CS | G11 | `pin_cs = 11` |
| SCK | G12 | `pin_sck = 12` |

出典: [Arduino チュートリアル](https://docs.m5stack.com/ja/arduino/projects/stamp/stamp_uwb)、[M5Stamp_UWB_Types.h](https://github.com/m5stack/M5Stamp-UWB/blob/main/src/M5Stamp_UWB_Types.h)

## C5 背面 FPC 0.5mm-12P（ホスト側）

| Pin | C5 | UWB-F |
|---|---|---|
| 1 | 3V3 | 3V3 |
| 2 | 3V3 | 3V3 |
| 3 | G23 | GP7 |
| 4 | G0 | IRQ |
| 5 | G24 | WAKEUP |
| 6 | G25 | RSTn |
| 7 | GND | GND |
| 8 | G26 | MISO (CDO) |
| 9 | G27 | MOSI (CDI) |
| 10 | TXD (G11) | CSn |
| 11 | GND | GND |
| 12 | RXD (G12) | CLK |

出典: [Stamp-C5](https://docs.m5stack.com/en/core/Stamp-C5)

SPI は slow 2 MHz から fast 16 MHz（チップ上限 32 MHz）。`begin_spi=true`、`hard_reset_on_begin=true`。

## 組み立て注意

- FPC 逆差し禁止。焼損する。
- アンテナ領域の下にフレキを通さない。インピーダンスが変わって RF が劣化する。
- Stamp-S3 / S3A の背面 FPC にはつながない。ピン配列が違う。永久破損。
- C5 の 3.3 V 供給が 300 mA 以上なら出力に 100 µF 以上。UWB 単体は約 58 mA なので通常は不要。
- 電源リップルは 12 mVpp 以下。

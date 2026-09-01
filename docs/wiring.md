# 配線

Stamp UWB F（S017-F）を Stamp-C5 ノーマルの背面 0.5mm-12P に FPC ケーブルでつなぐ。C5 DIP は未所持。電気的ピンは DIP と同一。

## 信号

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

| Pin | C5 | UWB 相当 |
|---|---|---|
| 1 | 3V3 | 3V3 |
| 2 | 3V3 | 3V3 |
| 3 | G23 | GP7 |
| 4 | G0 | IRQ |
| 5 | G24 | WAKEUP |
| 6 | G25 | RST |
| 7 | GND | GND |
| 8 | G26 | MISO (CDO) |
| 9 | G27 | MOSI (CDI) |
| 10 | TXD (G11) | CS |
| 11 | GND | GND |
| 12 | RXD (G12) | SCK |

出典: [Stamp-C5](https://docs.m5stack.com/en/core/Stamp-C5)

SPI は slow 2 MHz から fast 16 MHz（チップ上限 32 MHz）。`begin_spi=true`、`hard_reset_on_begin=true`。

## 組み立て注意

- FPC 逆差し禁止。焼損する。
- アンテナ領域の下にフレキを通さない。インピーダンスが変わって RF が劣化する。
- Stamp-S3 / S3A の背面 FPC にはつながない。ピン配列が違う。永久破損。
- C5 の 3.3 V 供給が 300 mA 以上なら出力に 100 µF 以上。UWB 単体は約 58 mA なので通常は不要。
- 電源リップルは 12 mVpp 以下。

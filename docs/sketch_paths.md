# スケッチパスの設定

`sketches/` 内のスケッチは `include/` のヘッダ（`uwb_pins.h`, `uwb_addrs.h`, `uwb_phy.h`）を参照する。Arduino IDE でスケッチを開く場合、インクルードパスを設定する必要がある。

## 方法 1: ヘッダをスケッチフォルダにコピー（推奨）

Arduino IDE で最も簡単な方法。

```bash
# DS_TWR_TAG の場合
cp include/*.h sketches/DS_TWR_TAG/

# DS_TWR_ANCHOR の場合
cp include/*.h sketches/DS_TWR_ANCHOR/
```

これでスケッチを Arduino IDE で直接開いてコンパイルできる。

**注意**: `include/` のヘッダを変更した場合は、スケッチフォルダにも再コピーすること。

## 方法 2: シンボリックリンク

Unix 系 OS（macOS, Linux）の場合、シンボリックリンクでも可。

```bash
cd sketches/DS_TWR_TAG
ln -s ../../include/uwb_pins.h .
ln -s ../../include/uwb_addrs.h .
ln -s ../../include/uwb_phy.h .
```

Windows では管理者権限が必要なため非推奨。

## 方法 3: Arduino CLI（CI 向け）

GitHub Actions CI では `--build-property` でインクルードパスを追加している。

```bash
arduino-cli compile \
  --fqbn m5stack:esp32:M5StampC5 \
  --build-property "build.extra_flags=-I/path/to/repo/include" \
  sketches/DS_TWR_TAG
```

この方法はスケッチフォルダにヘッダをコピーする必要がない。

## ヘッダの役割

| ヘッダ | 内容 |
|---|---|
| `uwb_pins.h` | Stamp UWB F → Stamp-C5 のピンマッピング |
| `uwb_addrs.h` | PAN ID、タグ・アンカーのショートアドレス |
| `uwb_phy.h` | PHY 設定（チャンネル、プリアンブル長、送信電力、アンテナ遅延） |

これらの値を変更する場合は `include/` のヘッダを編集し、スケッチフォルダにコピーし直す（方法 1 の場合）。

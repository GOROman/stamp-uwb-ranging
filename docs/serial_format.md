# シリアル出力

ボーレート 115200。

## 2ノード DS-TWR（Tag）

成功時の例（公式。チャネルは GitHub 既定なら `ch=9`。下の `ch=5` はドキュメント例）：

```
M5Stamp UWB DS-TWR TAG
ROLE,mode=TAG
TWR_MODE,mode=DS-TWR
UWB_ID,dev_id=0xDECA0314,chip=QM33120/DW3720
UWB_CONFIG,result=OK,ch=9,plen=128,rate=6M8,tx_power=0xFEFEFEFE
TEST_START,result=OK
DS_RANGE_STAT,count=10,ok=10,fail=0,last=OK,seq=10,distance_mm=164,distance_m=0.164,elapsed_ms=7
```

- チップ ID 期待値: `0xDECA0314`（表示名 `QM33120/DW3720`）
- Tag は 10 回に 1 行（`TAG_LOG_INTERVAL=10`）
- 距離は `distance_mm`（int32）と `distance_m`（float）
- `elapsed_ms` は 1 交換の所要（例 7 ms）
- 失敗は `last=FAIL,error=%s`

## Anchor

`DS_RESP_STAT,... requester=0x1,distance_mm=...,elapsed_ms=98`（20 回ごと）。
Idle の `RxTimeout` は失敗に数えない。

## 間隔（仕様ではなく例スケッチの定数）

- 2ノード DS: `RANGE_INTERVAL_MS = 200`（試行 5 Hz）
- GitHub マルチ: `POLL_INTERVAL_MS = 200`、Anchor 間 `ANCHOR_GAP_MS = 20`
- ドキュメント マルチ: `RANGE_INTERVAL_MS = 1000`

/*
 * DS-TWR タグ（イニシエーター）スケッチ
 *
 * M5Stamp C5 + Stamp UWB F で DS-TWR 測距を行う。
 * 公式 M5Stamp-UWB ライブラリの DS_TWR_TAG 例に基づく。
 *
 * ピン・アドレス・PHY 設定は include/ のヘッダを参照。
 * Arduino IDE で使う場合は sketches/DS_TWR_TAG/ にヘッダをコピーするか、
 * CI のように --build-property でインクルードパスを追加する。
 *
 * SPDX-License-Identifier: MIT
 */
#include <Arduino.h>
#include <M5Stamp_UWB.h>

// プロジェクト共通ヘッダ（include/ からコピーまたはパス追加）
#include "uwb_pins.h"
#include "uwb_addrs.h"
#include "uwb_phy.h"

// -----------------------------------------------------------------------------
// UWB ネットワーク・測距パラメータ
// -----------------------------------------------------------------------------
// RX/ホストタイムアウト。UWB タイムアウトは UWB マイクロ秒 (uus)、
// hostTimeoutMs は MCU がドライバステータスをポーリングする最大時間。
static constexpr uint32_t RX_TIMEOUT_UUS = 3000;
static constexpr uint32_t RANGE_HOST_TIMEOUT_MS = 100;

// DS 応答タイミング。TAG は Poll 送信後に RX を開く。
// ANCHOR は RESPONSE_TX_DLY_UUS で遅延 Response を送信。
static constexpr uint32_t RESPONSE_RX_AFTER_TX_DLY_UUS = 1500;
static constexpr uint32_t RESPONSE_TX_DLY_UUS = 3000;

// DS 専用タイミング。Response 受信後、TAG は Final を送信。
// ANCHOR は Final を待ち、計算した距離を Result フレームで返す。
static constexpr uint32_t FINAL_TX_DLY_UUS = 1800;
static constexpr uint32_t FINAL_RX_AFTER_RESPONSE_TX_DLY_UUS = 500;
static constexpr uint32_t RESULT_RX_AFTER_FINAL_TX_DLY_UUS = 500;

// ANCHOR は Result フレームを複数回送信して受信成功率を上げる。
static constexpr uint8_t RESULT_REPEAT_COUNT = 3;
static constexpr uint32_t RESULT_REPEAT_GAP_MS = 3;

M5Stamp_UWB uwb;
static bool uwbReady = false;

// TAG は RANGE_INTERVAL_MS ごとに測距を開始。
static constexpr uint32_t RANGE_INTERVAL_MS = 200;
// TAG は TAG_LOG_INTERVAL 回ごとにログを出力。失敗は次の統計行でまとめて表示。
static constexpr uint32_t TAG_LOG_INTERVAL = 10;
static uint32_t lastRangeMs = 0;
static uint32_t rangeCount = 0;
static uint32_t rangeOkCount = 0;
static uint32_t rangeFailCount = 0;

static M5Stamp_UWBDSRangeConfig makeDSRangeConfig()
{
    M5Stamp_UWBDSRangeConfig range;
    range.panId = UWB_PAN_ID;
    range.initiatorAddress = UWB_ADDR_TAG;
    range.responderAddress = UWB_ADDR_ANCHOR_2;
    range.responseRxAfterTxDelayUus = RESPONSE_RX_AFTER_TX_DLY_UUS;
    range.responseTxDelayUus = RESPONSE_TX_DLY_UUS;
    range.finalTxDelayUus = FINAL_TX_DLY_UUS;
    range.finalRxAfterResponseTxDelayUus = FINAL_RX_AFTER_RESPONSE_TX_DLY_UUS;
    range.resultRxAfterFinalTxDelayUus = RESULT_RX_AFTER_FINAL_TX_DLY_UUS;
    range.rxTimeoutUus = RX_TIMEOUT_UUS;
    range.hostTimeoutMs = RANGE_HOST_TIMEOUT_MS;
    range.resultRepeatCount = RESULT_REPEAT_COUNT;
    range.resultRepeatGapMs = RESULT_REPEAT_GAP_MS;
    return range;
}

static bool initUwb()
{
    // M5Stamp_UWBConfig にピン設定を反映
    M5Stamp_UWBConfig config;
    config.pin_cs = UWB_PIN_CS;
    config.pin_rst = UWB_PIN_RST;
    config.pin_irq = UWB_PIN_IRQ;
    config.pin_wakeup = UWB_PIN_WAKEUP;
    config.pin_gp7 = UWB_PIN_GP7;
    config.pin_sck = UWB_PIN_SCK;
    config.pin_miso = UWB_PIN_MISO;
    config.pin_mosi = UWB_PIN_MOSI;

    // PHY はライブラリ既定（Channel 9）を使用
    M5Stamp_UWBPHYConfig phy;

    if (!uwb.begin(config, phy)) {
        Serial.printf("UWB_BEGIN,result=FAIL,error=%s\n", uwb.lastErrorName());
        Serial.printf("UWB_RAW_ID,dev_id=0x%08lX\n", static_cast<unsigned long>(uwb.readRawDeviceId()));
        return false;
    }

    const uint32_t devId = uwb.deviceId();
    Serial.printf("UWB_ID,dev_id=0x%08lX,chip=%s\n", static_cast<unsigned long>(devId), uwb.chipName());
    if (devId != M5STAMP_UWB_QM33120_DEVICE_ID) {
        Serial.printf("UWB_ID,result=FAIL,expected=0xDECA0314\n");
        return false;
    }

    Serial.printf("UWB_CONFIG,result=OK,ch=%u,plen=%u,rate=6M8,tx_power=0x%08lX\n",
                  static_cast<unsigned>(phy.channel),
                  static_cast<unsigned>(phy.preambleLength),
                  static_cast<unsigned long>(phy.txPower));
    return true;
}

static void runTagRole()
{
    if ((millis() - lastRangeMs) < RANGE_INTERVAL_MS) {
        return;
    }
    lastRangeMs = millis();

    const M5Stamp_UWBDSRangeResult result = uwb.requestDSRange(makeDSRangeConfig());
    const char* logPrefix = "DS_RANGE_STAT";

    rangeCount++;
    if (result.success) {
        rangeOkCount++;
    } else {
        rangeFailCount++;
    }

    if ((rangeCount % TAG_LOG_INTERVAL) != 0) {
        return;
    }

    if (result.success) {
        Serial.printf("%s,count=%lu,ok=%lu,fail=%lu,last=OK,seq=%u,distance_mm=%ld,distance_m=%.3f,elapsed_ms=%lu\n",
                      logPrefix,
                      static_cast<unsigned long>(rangeCount),
                      static_cast<unsigned long>(rangeOkCount),
                      static_cast<unsigned long>(rangeFailCount),
                      result.sequence,
                      static_cast<long>(result.distanceMm),
                      result.distanceM,
                      static_cast<unsigned long>(result.elapsedMs));
    } else {
        Serial.printf("%s,count=%lu,ok=%lu,fail=%lu,last=FAIL,seq=%u,error=%s\n",
                      logPrefix,
                      static_cast<unsigned long>(rangeCount),
                      static_cast<unsigned long>(rangeOkCount),
                      static_cast<unsigned long>(rangeFailCount),
                      result.sequence,
                      uwb.lastErrorName());
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.printf("M5Stamp UWB DS-TWR TAG\n");
    Serial.printf("ROLE,mode=TAG\n");
    Serial.printf("TWR_MODE,mode=DS-TWR\n");
    Serial.printf("ADDR,pan=0x%04X,tag=0x%04X,anchor=0x%04X\n",
                  UWB_PAN_ID, UWB_ADDR_TAG, UWB_ADDR_ANCHOR_2);
    uwbReady = initUwb();
    Serial.printf("TEST_START,result=%s\n", uwbReady ? "OK" : "FAIL");
}

void loop()
{
    if (!uwbReady) {
        delay(1000);
        return;
    }
    runTagRole();
}

/*
 * DS-TWR アンカー（レスポンダー）スケッチ
 *
 * M5Stamp C5 + Stamp UWB F で DS-TWR 測距を行う。
 * 公式 M5Stamp-UWB ライブラリの DS_TWR_ANCHOR 例に基づく。
 *
 * ピン・アドレス・PHY 設定は include/ のヘッダを参照。
 * Arduino IDE で使う場合は sketches/DS_TWR_ANCHOR/ にヘッダをコピーするか、
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

// ANCHOR は ANCHOR_LOG_INTERVAL 回の成功ごとにログを出力。
// レスポンダーはログを控えめにして RX ホットパスを妨げない。
static constexpr uint32_t ANCHOR_LOG_INTERVAL = 20;

static uint32_t anchorRespCount = 0;
static uint32_t anchorFailCount = 0;

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

    // uwb_phy.h のマクロから PHY パラメータを設定
    M5Stamp_UWBPHYConfig phy;
    phy.channel = static_cast<M5Stamp_UWBChannel>(UWB_CHANNEL);
    phy.preambleLength = static_cast<M5Stamp_UWBPreambleLength>(UWB_PREAMBLE_LEN);
    phy.txPower = UWB_TX_POWER;
    phy.txAntennaDelay = UWB_ANT_DELAY_TX;
    phy.rxAntennaDelay = UWB_ANT_DELAY_RX;

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

static void runAnchorRole()
{
    const M5Stamp_UWBDSResponderResult result = uwb.respondDSRange(makeDSRangeConfig());
    const char* logPrefix = "DS_RESP_STAT";

    if (!result.success) {
        if (result.error == M5Stamp_UWBError::RxTimeout) {
            return;
        }

        anchorFailCount++;
        if ((anchorFailCount % ANCHOR_LOG_INTERVAL) == 0) {
            Serial.printf("%s,count=%lu,fail=%lu,last=FAIL,error=%s\n",
                          logPrefix,
                          static_cast<unsigned long>(anchorRespCount),
                          static_cast<unsigned long>(anchorFailCount),
                          uwb.lastErrorName());
        }
        return;
    }

    anchorRespCount++;
    if ((anchorRespCount % ANCHOR_LOG_INTERVAL) != 0) {
        return;
    }

    Serial.printf("%s,count=%lu,fail=%lu,last=OK,seq=%u,requester=0x%X,distance_mm=%ld,distance_m=%.3f,elapsed_ms=%lu\n",
                  logPrefix,
                  static_cast<unsigned long>(anchorRespCount),
                  static_cast<unsigned long>(anchorFailCount),
                  result.sequence,
                  result.requester,
                  static_cast<long>(result.distanceMm),
                  result.distanceM,
                  static_cast<unsigned long>(result.elapsedMs));
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.printf("M5Stamp UWB DS-TWR ANCHOR\n");
    Serial.printf("ROLE,mode=ANCHOR\n");
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
    runAnchorRole();
}

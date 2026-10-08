#ifndef CURVA_DIAGNOSTIC_H
#define CURVA_DIAGNOSTIC_H

#include <stdint.h>
#include "cycfg_capsense.h"

#define DIAG_HISTORY_LENGTH (32u)
#define DIAG_MAGIC (0x43535644u) /* CSVD */
#define DIAG_VERIFY_PASSES (4u)

typedef struct {
    uint16_t scanRaw;
    uint16_t raw;
    uint16_t baseline;
    uint16_t diff;
    uint16_t scaled;
    uint8_t status;
    uint8_t cdacComp;
} diag_sensor_t;

typedef struct {
    uint16_t maxRaw;
    uint16_t snsClk;
    uint16_t nSub;
    uint8_t cdacRef;
    uint8_t reserved;
} diag_tuning_t;

typedef struct {
    uint32_t statusOr;
    uint16_t preVerifyRaw;
    uint16_t firstRaw;
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t maxRawCount;
    uint8_t cdacRef;
    uint8_t failureCount;
} diag_verify_t;

typedef struct {
    uint32_t sequence; /* Odd while being written, even when complete. */
    uint32_t scanStartMs;
    uint32_t scanEndMs;
    uint32_t processEndMs;
    uint32_t txStartMs;
    diag_sensor_t sensor[CY_CAPSENSE_WIDGET_COUNT];
} diag_frame_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t widgetCount;
    uint32_t milliseconds;
    uint32_t frameCount;
    uint32_t scanErrorCount;
    uint32_t processErrorCount;
    uint32_t uartErrorCount;
    uint32_t initStage;
    uint32_t lastCapsenseStatus;
    uint32_t bistEnabled;
    uint32_t bistStatus[CY_CAPSENSE_WIDGET_COUNT];
    uint32_t electrodeCapFf[CY_CAPSENSE_WIDGET_COUNT];
    diag_tuning_t tuning[CY_CAPSENSE_WIDGET_COUNT];
    diag_frame_t frame[DIAG_HISTORY_LENGTH];
    /* Appended so existing diagnostic field offsets remain unchanged. */
    uint32_t verifyStage; /* 0: not run, 1: running, 2: complete. */
    uint32_t verifyPasses;
    diag_verify_t verify[CY_CAPSENSE_WIDGET_COUNT];
} diag_state_t;

/* Read this symbol with the debugger. It is not transmitted on UART. */
extern volatile diag_state_t g_diag;

void Diag_Init(void);
void Diag_CaptureTuning(void);
void Diag_RunBistOnce(void);
void Diag_VerifyAfterInitFailure(void);
void Diag_RecordFrame(uint32_t scanStartMs, uint32_t scanEndMs,
                      uint32_t processEndMs, const uint16_t *scanRaw,
                      const uint8_t *txBuffer);
void Diag_MarkTxStart(void);
uint32_t Diag_Millis(void);

#endif

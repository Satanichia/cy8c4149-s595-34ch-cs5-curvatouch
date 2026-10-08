#include "diagnostic.h"
#include "cy_pdl.h"

volatile diag_state_t g_diag;

static void Diag_Tick(void)
{
    g_diag.milliseconds++;
}

uint32_t Diag_Millis(void)
{
    return g_diag.milliseconds;
}

void Diag_Init(void)
{
    g_diag.magic = DIAG_MAGIC;
    g_diag.version = 2u;
    g_diag.widgetCount = CY_CAPSENSE_WIDGET_COUNT;
    /* This project does not otherwise use SysTick. One tick is 1 ms. */
    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, SystemCoreClock / 1000u - 1u);
    (void)Cy_SysTick_SetCallback(0u, Diag_Tick);
}

void Diag_CaptureTuning(void)
{
    for (uint32_t i = 0u; i < CY_CAPSENSE_WIDGET_COUNT; ++i) {
        const cy_stc_capsense_widget_context_t *w = &cy_capsense_tuner.widgetContext[i];
        g_diag.tuning[i].maxRaw = w->maxRawCount;
        g_diag.tuning[i].snsClk = w->snsClk;
        g_diag.tuning[i].nSub = w->numSubConversions;
        g_diag.tuning[i].cdacRef = w->cdacRef;
    }
}

void Diag_RunBistOnce(void)
{
#if (CY_CAPSENSE_BIST_EN != 0u) && (CY_CAPSENSE_TST_ELTD_CAP_EN != 0u)
    g_diag.bistEnabled = 1u;
    for (uint32_t i = 0u; i < CY_CAPSENSE_WIDGET_COUNT; ++i) {
        g_diag.bistStatus[i] = (uint32_t)Cy_CapSense_MeasureCapacitanceSensorElectrode(
            i, 0u, &cy_capsense_context);
        if (g_diag.bistStatus[i] == (uint32_t)CY_CAPSENSE_BIST_SUCCESS_E) {
            g_diag.electrodeCapFf[i] = cy_capsense_context.ptrWdConfig[i].ptrEltdCapacitance[0];
        }
    }
#else
    g_diag.bistEnabled = 0u;
#endif
}

void Diag_VerifyAfterInitFailure(void)
{
    /* Preserve the values left by the failed initialization before any
     * diagnostic verification scan can overwrite the sensor contexts. */
    for (uint32_t i = 0u; i < CY_CAPSENSE_WIDGET_COUNT; ++i) {
        const cy_stc_capsense_widget_config_t *w = &cy_capsense_context.ptrWdConfig[i];
        volatile diag_verify_t *v = &g_diag.verify[i];
        v->preVerifyRaw = w->ptrSnsContext[0].raw;
        v->maxRawCount = w->ptrWdContext->maxRawCount;
        v->cdacRef = w->ptrWdContext->cdacRef;
        v->minRaw = UINT16_MAX;
    }

    g_diag.verifyStage = 1u;
    for (uint32_t pass = 0u; pass < DIAG_VERIFY_PASSES; ++pass) {
        for (uint32_t i = 0u; i < CY_CAPSENSE_WIDGET_COUNT; ++i) {
            volatile diag_verify_t *v = &g_diag.verify[i];
            const uint32_t status = (uint32_t)Cy_CapSense_VerifyCalibration(
                i, &cy_capsense_context);
            const uint16_t raw = cy_capsense_context.ptrWdConfig[i].ptrSnsContext[0].raw;

            v->statusOr |= status;
            if (status != (uint32_t)CY_CAPSENSE_STATUS_SUCCESS) {
                v->failureCount++;
            }
            if (pass == 0u) {
                v->firstRaw = raw;
            }
            if (raw < v->minRaw) {
                v->minRaw = raw;
            }
            if (raw > v->maxRaw) {
                v->maxRaw = raw;
            }
        }
        g_diag.verifyPasses = pass + 1u;
    }
    g_diag.verifyStage = 2u;
}

void Diag_RecordFrame(uint32_t scanStartMs, uint32_t scanEndMs,
                      uint32_t processEndMs, const uint16_t *scanRaw,
                      const uint8_t *txBuffer)
{
    uint32_t slot = g_diag.frameCount % DIAG_HISTORY_LENGTH;
    volatile diag_frame_t *f = &g_diag.frame[slot];
    uint32_t sequence = (g_diag.frameCount + 1u) * 2u;

    f->sequence = sequence - 1u;
    __DMB();
    f->scanStartMs = scanStartMs;
    f->scanEndMs = scanEndMs;
    f->processEndMs = processEndMs;
    f->txStartMs = 0u;
    for (uint32_t i = 0u; i < CY_CAPSENSE_WIDGET_COUNT; ++i) {
        const cy_stc_capsense_sensor_context_t *s =
            &cy_capsense_context.ptrWdConfig[i].ptrSnsContext[0];
        f->sensor[i].scanRaw = scanRaw[i];
        f->sensor[i].raw = s->raw;
        f->sensor[i].baseline = s->bsln;
        f->sensor[i].diff = s->diff;
        f->sensor[i].scaled = (uint16_t)txBuffer[1u + 2u * i] |
                              ((uint16_t)txBuffer[2u + 2u * i] << 8u);
        f->sensor[i].status = s->status;
        f->sensor[i].cdacComp = s->cdacComp;
    }
    __DMB();
    f->sequence = sequence;
    g_diag.frameCount++;
}

void Diag_MarkTxStart(void)
{
    if (g_diag.frameCount != 0u) {
        volatile diag_frame_t *f =
            &g_diag.frame[(g_diag.frameCount - 1u) % DIAG_HISTORY_LENGTH];
        uint32_t sequence = f->sequence;
        f->sequence = sequence + 1u;
        __DMB();
        f->txStartMs = Diag_Millis();
        __DMB();
        f->sequence = sequence + 2u;
    }
}

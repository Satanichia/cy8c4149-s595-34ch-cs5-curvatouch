/******************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for the Empty PSOC4  Application
 *              for ModusToolbox.
 *
 * Related Document: See README.md
 *
 *
 ******************************************************************************
 * (c) 2020-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 *****************************************************************************/

#include <stdio.h>
#include <stdbool.h>

#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"


#define CY_MSC0_INTR_PRIOR (2u)
#define CY_MSC1_INTR_PRIOR (2u)
#define SCB4_UART_INTR_PRIOR (3u)

#define APP_LED_STATUS_ON (0u)
#define APP_LED_STATUS_OFF (1u)
  /* 2 bytes per widget + 1 start byte + 1 checksum byte */
#define APP_TX_BUFFER_LENGTH (CY_CAPSENSE_WIDGET_COUNT * 2 + 2)
#define APP_TX_BUFFER_COUNT (2u)
#define APP_SCALE_TARGET (4095ul)
#define APP_SCALE_MOVE (2u)

#define APP_CAPSENSE_SUBCONV_LIMIT_ENABLE (1u)
#define APP_CAPSENSE_SUBCONV_LIMIT_VALUE (16000ul)


static cy_stc_scb_uart_context_t uart4_context;

static uint8_t gs_uartTxBuffer[APP_TX_BUFFER_COUNT][APP_TX_BUFFER_LENGTH]; // Dual buffer
static uint16_t gs_widgetMaxValues[CY_CAPSENSE_WIDGET_COUNT];
static uint32_t gs_widgetGainValues[CY_CAPSENSE_WIDGET_COUNT];

static void App_UART4_IntrHandler(void) {
    /* Handle the UART4 interrupt */
    Cy_SCB_UART_Interrupt(SCB4_UART_HW, &uart4_context);
}

static void App_MSC0_IntrHandler(void) {
    /* Handle the MSC0 interrupt */
    Cy_CapSense_InterruptHandler(CY_MSC0_HW, &cy_capsense_context);
}

static void App_MSC1_IntrHandler(void) {
    /* Handle the MSC1 interrupt */
    Cy_CapSense_InterruptHandler(CY_MSC1_HW, &cy_capsense_context);
}

static bool UART_Init(void){
    
    const cy_stc_sysint_t UART_IntrConfig = {
        .intrSrc = SCB4_UART_IRQ,
        .intrPriority = SCB4_UART_INTR_PRIOR
    };

    if (CY_SCB_UART_SUCCESS != Cy_SCB_UART_Init(SCB4_UART_HW, &SCB4_UART_config, &uart4_context)) {
        return false;
    }

    /* Initialize and enable interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&UART_IntrConfig, App_UART4_IntrHandler)) {
        return false;
    }

    NVIC_ClearPendingIRQ(UART_IntrConfig.intrSrc);
    NVIC_EnableIRQ(UART_IntrConfig.intrSrc);

    Cy_SCB_UART_Enable(SCB4_UART_HW);

    return true;

}

static bool UART_IsTxActive(void) {
    return ((Cy_SCB_UART_GetTransmitStatus(SCB4_UART_HW, &uart4_context) & CY_SCB_UART_TRANSMIT_ACTIVE) != 0u);
}

static void CapSense_SubConvLimitCallback(void *context){
#if APP_CAPSENSE_SUBCONV_LIMIT_ENABLE
    uint32_t count = 0;
    uint32_t integration = 0;
    cy_stc_capsense_context_t *cctx = (cy_stc_capsense_context_t *)context;
    const cy_stc_capsense_widget_config_t *wcfg;
    cy_stc_capsense_widget_context_t *wctx;

    if (cctx == NULL) {
        return;
    }

    for (count=0; count < cctx->ptrCommonConfig->numWd; ++count) {
        wcfg = &cctx->ptrWdConfig[count];
        wctx = wcfg->ptrWdContext;

        // Condition filter
        if ((CY_CAPSENSE_CSD_GROUP != wcfg->senseMethod) || 
            (0u == Cy_CapSense_IsWidgetEnabled(count, cctx)) ||
            (64u != wctx->cicRate) ||
            (CY_CAPSENSE_CLK_SOURCE_DIRECT != wctx->snsClkSource)
        ) {
            continue;
        }

        integration = (uint32_t)wctx->numSubConversions * (uint32_t)wctx->snsClk;
        if (integration > APP_CAPSENSE_SUBCONV_LIMIT_VALUE && wctx->numSubConversions>=2u) {
            wctx->numSubConversions = (uint16_t)(wctx->numSubConversions / 2u);
        }
    }
#else
    (void)context;
#endif
}

static bool CapSense_Init(void) {

    const cy_stc_sysint_t MSC0_IntrConfig = {
        .intrSrc = CY_MSC0_IRQ,
        .intrPriority = CY_MSC0_INTR_PRIOR
    };

    const cy_stc_sysint_t MSC1_IntrConfig = {
        .intrSrc = CY_MSC1_IRQ,
        .intrPriority = CY_MSC1_INTR_PRIOR
    };

    if (CY_CAPSENSE_STATUS_SUCCESS != Cy_CapSense_Init(&cy_capsense_context)) {
        return false;
    }

    /* Perform integration limit */
    cy_capsense_context.ptrInternalContext->ptrEODsInitCallback = CapSense_SubConvLimitCallback;

    /* Initialize and enable interrupt */
    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&MSC0_IntrConfig, App_MSC0_IntrHandler)) {
        return false;
    }

    if (CY_SYSINT_SUCCESS != Cy_SysInt_Init(&MSC1_IntrConfig, App_MSC1_IntrHandler)) {
        return false;
    }

    NVIC_ClearPendingIRQ(MSC0_IntrConfig.intrSrc);
    NVIC_EnableIRQ(MSC0_IntrConfig.intrSrc);

    NVIC_ClearPendingIRQ(MSC1_IntrConfig.intrSrc);
    NVIC_EnableIRQ(MSC1_IntrConfig.intrSrc);
    
    if (CY_CAPSENSE_STATUS_SUCCESS != Cy_CapSense_Enable(&cy_capsense_context)) {
        return false;
    }

    return true;
}

static void CapSense_ProcessWidgetAuxValues(void){
    uint16_t max;
    for (uint8_t i = 0; i < CY_CAPSENSE_WIDGET_COUNT; i++) {
        max = cy_capsense_tuner.widgetContext[i].maxRawCount;
        gs_widgetMaxValues[i] = max;
        if(max == 0u){
            gs_widgetGainValues[i] = 0u;
        } else {
            gs_widgetGainValues[i] = (((uint32_t)APP_SCALE_TARGET << 16u) + (max / 2u)) / max;
        }
    }
}

inline static uint16_t CapSense_ScaleRawByGain(uint16_t raw, uint8_t widget) {
    uint16_t max = gs_widgetMaxValues[widget];
    uint32_t gain = gs_widgetGainValues[widget];
    uint32_t scaled;

    if(max == 0u || gain == 0u){
        return 0u;
    }
    scaled = (raw > max) ? max : raw;

    // ALGO: scaled = round(min(raw, maxRaw) × 4095 / maxRaw) × 16

    return (((uint32_t)scaled * gain + 0x8000u) >> 16u) << 4u;
}

inline static uint16_t CapSense_ScaleRawByBitMove(uint16_t raw) {
    return raw << APP_SCALE_MOVE;
}

static uint16_t CapSense_ScaleRaw(uint16_t raw, uint8_t widget) {
    return CapSense_ScaleRawByGain(raw, widget);
    // return CapSense_ScaleRawByBitMove(raw);
}

static void UART_FillBufferFromWidgets(uint8_t* dst) {
    uint8_t checksum = 0;
    uint16_t payload;
    uint8_t payloadLSB;
    uint8_t payloadMSB;
    const cy_stc_capsense_sensor_context_t *sensor;

    dst[0] = 0x0u; // Start byte

    for (uint8_t i = 0; i < CY_CAPSENSE_WIDGET_COUNT; i++) {
        sensor = &cy_capsense_context.ptrWdConfig[i].ptrSnsContext[0];
        payload = CapSense_ScaleRaw(sensor->raw, i);
        payloadLSB =  (uint8_t)(payload & 0xFFu); // Low byte
        payloadMSB = (uint8_t)((payload >> 8u) & 0xFFu); // High byte

        dst[1 + (i * 2)] = payloadLSB;
        dst[2 + (i * 2)] = payloadMSB;

        checksum = (uint8_t)(checksum + payloadLSB + payloadMSB);
    }
    dst[APP_TX_BUFFER_LENGTH - 1] = checksum;
}


/* -------- main() -------- */
int main(void) {
    uint8_t txBufferIdx = 0;
    // cy_capsense_status_t capsenseStatus;
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (CY_RSLT_SUCCESS != result) {
        CY_ASSERT(0);
    }
    
    /* Enable global interrupts */
    __enable_irq();

    /* Turn on LED at startup */
    Cy_GPIO_Write(LED_STATUS_PORT, LED_STATUS_PIN, APP_LED_STATUS_ON);

    if(!UART_Init()) {
        CY_ASSERT(0);
    }

    if(!CapSense_Init()) {
        CY_ASSERT(0);
    }

    CapSense_ProcessWidgetAuxValues();

    /* Begin first scan */
    if (CY_CAPSENSE_STATUS_SUCCESS != Cy_CapSense_ScanAllSlots(&cy_capsense_context)) {
        CY_ASSERT(0);
    }

    for (;;) {

        /* Phase 1-A: Waiting for the scan to complete */
        while (CY_CAPSENSE_NOT_BUSY != Cy_CapSense_IsBusy(&cy_capsense_context)) {
            /* Recheck with IRQs masked to avoid sleeping after the final ISR.
             * Without diagnostic SysTick there may be no later wake-up IRQ.
             */
            uint32_t irqState = Cy_SysLib_EnterCriticalSection();
            if (CY_CAPSENSE_NOT_BUSY != Cy_CapSense_IsBusy(&cy_capsense_context)) {
                __WFI();
            }
            Cy_SysLib_ExitCriticalSection(irqState);
        }

        /* Phase 1-B: Process widgets before next scan */
        // capsenseStatus = Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);
        // if (CY_CAPSENSE_STATUS_SUCCESS != capsenseStatus) {
        //     CY_ASSERT(0);
        // }
        Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

        /* Phase 1-C: Fill buffer from scan values */
        UART_FillBufferFromWidgets(gs_uartTxBuffer[txBufferIdx]);

        /* Phase 1-D: Begin next scan */
        // capsenseStatus = Cy_CapSense_ScanAllSlots(&cy_capsense_context);
        // if (CY_CAPSENSE_STATUS_SUCCESS != capsenseStatus) {
        //     CY_ASSERT(0);
        // }
        Cy_CapSense_ScanAllSlots(&cy_capsense_context);

        /* Phase 2-A: Waiting for last transmit to complete */
        while (UART_IsTxActive()){
            uint32_t irqState = Cy_SysLib_EnterCriticalSection();
            if (UART_IsTxActive()) {
                __WFI();
            }
            Cy_SysLib_ExitCriticalSection(irqState);
        }

        /* Phase 2-B: Begin transmit */
        // if (CY_SCB_UART_SUCCESS != Cy_SCB_UART_Transmit(SCB4_UART_HW,
        //         gs_uartTxBuffer[txBufferIdx], APP_TX_BUFFER_LENGTH, &uart4_context)) {
        //     CY_ASSERT(0);
        // }
        Cy_SCB_UART_Transmit(SCB4_UART_HW, 
                             gs_uartTxBuffer[txBufferIdx], 
                             APP_TX_BUFFER_LENGTH, 
                             &uart4_context);
        txBufferIdx ^= 1u;

        /* Phase 3: Toggle LED after one cycle completes */
        Cy_GPIO_Inv(LED_STATUS_PORT, LED_STATUS_PIN);
    }
}



/* [] END OF FILE */

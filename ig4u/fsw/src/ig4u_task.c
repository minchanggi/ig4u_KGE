/**
 * @file ig4u_task.c
 * @brief iG4U 추력 유닛 cFS 메인 태스크
 *
 * PAYLGBAT 패턴 준수: child task 없음.
 * SCH → WAKEUP_MID: 주기적 HK 폴링 (RS-422 Request HK Data)
 * SCH → CMD_MID:    지상 명령 처리
 */

#include "ig4u_task.h"
#include "ig4u_dispatch.h"
#include "ig4u_eventids.h"
#include "ig4u_hal.h"
#include <string.h>

/* 전역 앱 데이터 */
IG4U_Data_t IG4U_Data;


/* ===================================================================
 * 메인 엔트리
 * =================================================================== */
void IG4U_Main(void)
{
    CFE_Status_t     Status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(IG4U_PERF_ID);

    Status = IG4U_Init();
    if (Status != CFE_SUCCESS)
    {
        IG4U_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&IG4U_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(IG4U_PERF_ID);

        Status = CFE_SB_ReceiveBuffer(&SBBufPtr, IG4U_Data.CmdPipe, CFE_SB_PEND_FOREVER);

        CFE_ES_PerfLogEntry(IG4U_PERF_ID);

        if (Status == CFE_SUCCESS)
        {
            IG4U_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(IG4U_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: SB Pipe Read Error. RC=0x%08lX",
                              (unsigned long)Status);
            IG4U_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(IG4U_PERF_ID);
    CFE_ES_ExitApp(IG4U_Data.RunStatus);
}


/* ===================================================================
 * 초기화
 * =================================================================== */
CFE_Status_t IG4U_Init(void)
{
    CFE_Status_t Status;

    memset(&IG4U_Data, 0, sizeof(IG4U_Data));
    IG4U_Data.RunStatus   = CFE_ES_RunStatus_APP_RUN;
    IG4U_Data.PipeDepth   = IG4U_PIPE_DEPTH;
    IG4U_Data.CurrentState = IG4U_STATE_BOOT;

    strncpy(IG4U_Data.CmdPipeName, "IG4U_CMD_PIPE", sizeof(IG4U_Data.CmdPipeName));
    IG4U_Data.CmdPipeName[sizeof(IG4U_Data.CmdPipeName) - 1] = '\0';

    /* EVS 등록 */
    Status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (Status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("IG4U: EVS Register failed. RC=0x%08lX\n",
                             (unsigned long)Status);
        return Status;
    }

    /* TLM 메시지 초기화 */
    CFE_MSG_Init(CFE_MSG_PTR(IG4U_Data.HkTlm.TelemetryHeader),
                 CFE_SB_ValueToMsgId(IG4U_HK_TLM_MID),
                 sizeof(IG4U_Data.HkTlm));

    CFE_MSG_Init(CFE_MSG_PTR(IG4U_Data.StatusTlm.TelemetryHeader),
                 CFE_SB_ValueToMsgId(IG4U_STATUS_TLM_MID),
                 sizeof(IG4U_Data.StatusTlm));

    CFE_MSG_Init(CFE_MSG_PTR(IG4U_Data.FireResultTlm.TelemetryHeader),
                 CFE_SB_ValueToMsgId(IG4U_FIRE_RESULT_MID),
                 sizeof(IG4U_Data.FireResultTlm));

    CFE_MSG_Init(CFE_MSG_PTR(IG4U_Data.FaultLogTlm.TelemetryHeader),
                 CFE_SB_ValueToMsgId(IG4U_FAULT_LOG_MID),
                 sizeof(IG4U_Data.FaultLogTlm));

    /* SB 파이프 생성 */
    Status = CFE_SB_CreatePipe(&IG4U_Data.CmdPipe, IG4U_Data.PipeDepth,
                                IG4U_Data.CmdPipeName);
    if (Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(IG4U_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: Create Pipe failed. RC=0x%08lX",
                          (unsigned long)Status);
        return Status;
    }

    /* 지상 명령 구독 */
    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(IG4U_CMD_MID), IG4U_Data.CmdPipe);
    if (Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(IG4U_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: Subscribe CMD failed. RC=0x%08lX",
                          (unsigned long)Status);
        return Status;
    }

    /* WAKEUP 구독 (SCH 주기 폴링 트리거) */
    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(IG4U_WAKEUP_MID), IG4U_Data.CmdPipe);
    if (Status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(IG4U_SUB_WAKEUP_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: Subscribe WAKEUP failed. RC=0x%08lX",
                          (unsigned long)Status);
        return Status;
    }

    /* RS-422 HAL 초기화 (CFE_SRL UART 핸들 획득 + 파라미터 설정) */
    {
        int32_t hal_ret = IG4U_HAL_Init();
        if (hal_ret != IG4U_HAL_OK)
        {
            CFE_EVS_SendEvent(IG4U_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: RS-422 HAL init failed. RC=%d", (int)hal_ret);
            return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
    }

    /* 초기 상태: BOOT → STANDBY (BIT 생략, 실제 구현 시 BIT 결과로 분기) */
    IG4U_Data.CurrentState = IG4U_STATE_STANDBY;
    IG4U_Data.PowerApplied = true;
    IG4U_Data.LinkReady    = true;

    CFE_EVS_SendEvent(IG4U_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U App Initialized. UART=%u bps, State=STANDBY.",
                      IG4U_UART_BAUD_RATE);

    return CFE_SUCCESS;
}

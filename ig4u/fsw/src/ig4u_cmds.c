/**
 * @file ig4u_cmds.c
 * @brief iG4U cFS 지상 명령 핸들러 구현
 *
 * 각 핸들러:
 *  1. 상태 유효성 검사 (현재 상태에서 허용된 명령인지)
 *  2. ig4u_commands.c Wire-level TC 함수 호출 (RS-422 송수신)
 *  3. 결과를 cFS TLM으로 SB 발행
 *  4. EVS 이벤트 전송
 */

#include "ig4u_task.h"
#include "ig4u_cmds.h"
#include "ig4u_commands.h"   /* Wire-level TC/TM 함수 */
#include "ig4u_eventids.h"
#include <string.h>

/* ===================================================================
 * 내부 헬퍼: 상태 허용 여부 검사
 * =================================================================== */

/** 추력기 구동 명령은 ARMED 상태에서만 허용 */
static bool IsArmed(void)
{
    return (IG4U_Data.CurrentState == IG4U_STATE_ARMED);
}

/** 통신 명령은 STANDBY 이상 (FAULT 제외)에서 허용 */
static bool IsCommAllowed(void)
{
    return (IG4U_Data.CurrentState == IG4U_STATE_STANDBY ||
            IG4U_Data.CurrentState == IG4U_STATE_ARMED   ||
            IG4U_Data.CurrentState == IG4U_STATE_SAFE);
}

/* ===================================================================
 * 시스템 명령
 * =================================================================== */

CFE_Status_t IG4U_NoopCmd(const IG4U_NoopCmd_t *Msg)
{
    (void)Msg;
    IG4U_Data.CmdCounter++;
    CFE_EVS_SendEvent(IG4U_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: NOOP command. Version 1.0. CmdCnt=%u",
                      IG4U_Data.CmdCounter);
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_ResetCounterCmd(const IG4U_ResetCounterCmd_t *Msg)
{
    (void)Msg;
    IG4U_Data.CmdCounter = 0U;
    IG4U_Data.ErrCounter = 0U;
    CFE_EVS_SendEvent(IG4U_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: Counters reset.");
    return CFE_SUCCESS;
}

/* ===================================================================
 * Group 1: 모듈 제어
 * =================================================================== */

CFE_Status_t IG4U_PingCmd(const IG4U_PingCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_Ping();
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.LinkReady = true;
        CFE_EVS_SendEvent(IG4U_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: PING OK. Link confirmed.");
    }
    else
    {
        IG4U_Data.ErrCounter++;
        IG4U_Data.LinkReady = false;
        CFE_EVS_SendEvent(IG4U_UART_TIMEOUT_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: PING failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_ResetModuleCmd(const IG4U_ResetModuleCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_ResetModule();
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.CurrentState    = IG4U_STATE_STANDBY;
        IG4U_Data.ArmedTimerActive = false;
        IG4U_Data.FaultFlags       = IG4U_FAULT_NONE;
        CFE_EVS_SendEvent(IG4U_STATE_STANDBY_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: Module reset. State -> STANDBY.");
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_SEND_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: RESET MODULE failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_SetModeCmd(const IG4U_SetModeCmd_t *Msg)
{
    int32_t ret;
    uint8_t mode;

    mode = Msg->Payload.Mode;
    ret  = IG4U_Cmd_SetMode(mode);

    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;

        /* 상태머신 동기화 */
        switch (mode)
        {
            case IG4U_MODE_STANDBY:
                IG4U_Data.CurrentState     = IG4U_STATE_STANDBY;
                IG4U_Data.ArmedTimerActive = false;
                CFE_EVS_SendEvent(IG4U_STATE_STANDBY_INF_EID, CFE_EVS_EventType_INFORMATION,
                                  "IG4U: State -> STANDBY.");
                break;

            case IG4U_MODE_ARMED:
                IG4U_Data.CurrentState     = IG4U_STATE_ARMED;
                IG4U_Data.ArmedEntryTime   = CFE_TIME_GetTime();
                IG4U_Data.ArmedTimerActive = true;
                CFE_EVS_SendEvent(IG4U_STATE_ARMED_INF_EID, CFE_EVS_EventType_INFORMATION,
                                  "IG4U: State -> ARMED. Timeout=%u sec.",
                                  IG4U_ARMED_TIMEOUT_SEC);
                break;

            case IG4U_MODE_SAFE:
                IG4U_Data.CurrentState     = IG4U_STATE_SAFE;
                IG4U_Data.ArmedTimerActive = false;
                CFE_EVS_SendEvent(IG4U_STATE_SAFE_ERR_EID, CFE_EVS_EventType_ERROR,
                                  "IG4U: State -> SAFE.");
                break;

            default:
                break;
        }
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_NACK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: SET MODE failed. Mode=%u RC=%d", mode, (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_ArmPropulsionCmd(const IG4U_ArmPropulsionCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    /* 현재 상태가 STANDBY여야 ARM 가능 */
    if (IG4U_Data.CurrentState != IG4U_STATE_STANDBY)
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_INTERLOCK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: ARM rejected. Current state=%u (must be STANDBY).",
                          IG4U_Data.CurrentState);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    ret = IG4U_Cmd_ArmPropulsion();
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.CurrentState     = IG4U_STATE_ARMED;
        IG4U_Data.ArmedEntryTime   = CFE_TIME_GetTime();
        IG4U_Data.ArmedTimerActive = true;
        CFE_EVS_SendEvent(IG4U_STATE_ARMED_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: ARM OK. State -> ARMED. Timeout=%u sec.",
                          IG4U_ARMED_TIMEOUT_SEC);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_INTERLOCK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: ARM failed (interlock). RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_DisarmPropulsionCmd(const IG4U_DisarmPropulsionCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_DisarmPropulsion();
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.CurrentState     = IG4U_STATE_STANDBY;
        IG4U_Data.ArmedTimerActive = false;
        CFE_EVS_SendEvent(IG4U_STATE_STANDBY_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: DISARM OK. State -> STANDBY.");
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_NACK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: DISARM failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

/* ===================================================================
 * Group 2: HK / 상태
 * =================================================================== */

CFE_Status_t IG4U_RequestHKDataCmd(const IG4U_RequestHKDataCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_RequestHKData(&IG4U_Data.LastHK);
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;

        /* TLM 페이로드 갱신 후 SB 발행 */
        IG4U_Data.HkTlm.Payload.CmdCounter  = IG4U_Data.CmdCounter;
        IG4U_Data.HkTlm.Payload.CmdErrCounter = IG4U_Data.ErrCounter;
        IG4U_Data.HkTlm.Payload.Mode          = IG4U_Data.LastHK.Mode;
        IG4U_Data.HkTlm.Payload.FaultFlags    = IG4U_Data.LastHK.FaultFlags_Lo |
                                                 ((uint16_t)IG4U_Data.LastHK.FaultFlags_Hi << 8U);
        memcpy(IG4U_Data.HkTlm.Payload.Pressure_kPa_x10,
               IG4U_Data.LastHK.Pressure_kPa_x10,
               sizeof(IG4U_Data.LastHK.Pressure_kPa_x10));
        memcpy(IG4U_Data.HkTlm.Payload.Temperature_x10,
               IG4U_Data.LastHK.Temperature_x10,
               sizeof(IG4U_Data.LastHK.Temperature_x10));
        IG4U_Data.HkTlm.Payload.MainFireCount = IG4U_Data.MainFireCount;
        IG4U_Data.HkTlm.Payload.CGFireCount   = IG4U_Data.CGFireCount;

        CFE_SB_TimeStampMsg(CFE_MSG_PTR(IG4U_Data.HkTlm.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(IG4U_Data.HkTlm.TelemetryHeader), true);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_RECV_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: REQUEST HK DATA failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_RequestStatusCmd(const IG4U_RequestStatusCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_RequestStatus(&IG4U_Data.LastStatus);
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;

        /* 상태머신 동기화 */
        IG4U_Data.CurrentState = (IG4U_State_t)IG4U_Data.LastStatus.Mode;
        IG4U_Data.FaultFlags   = IG4U_Data.LastStatus.FaultFlags;

        /* STATUS TLM 발행 */
        memcpy(&IG4U_Data.StatusTlm.Payload, &IG4U_Data.LastStatus,
               sizeof(IG4U_Data.LastStatus));
        CFE_SB_TimeStampMsg(CFE_MSG_PTR(IG4U_Data.StatusTlm.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(IG4U_Data.StatusTlm.TelemetryHeader), true);

        /* FAULT 상태 감지 시 이벤트 */
        if (IG4U_Data.CurrentState == IG4U_STATE_FAULT)
        {
            CFE_EVS_SendEvent(IG4U_STATE_FAULT_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: Status reports FAULT. FaultFlags=0x%04X",
                              IG4U_Data.FaultFlags);
        }
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_RECV_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: REQUEST STATUS failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

/* ===================================================================
 * Group 3: 주추력기
 * =================================================================== */

CFE_Status_t IG4U_MainThrusterFireCmd(const IG4U_MainThrusterFireCmd_t *Msg)
{
    int32_t                           ret;
    IG4U_MainThrusterResult_Payload_t result;

    if (!IsArmed())
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_INTERLOCK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: MAIN FIRE rejected. State=%u (must be ARMED).",
                          IG4U_Data.CurrentState);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    memset(&result, 0, sizeof(result));
    IG4U_Data.CurrentState     = IG4U_STATE_FIRING;
    IG4U_Data.ArmedTimerActive = false;

    CFE_EVS_SendEvent(IG4U_MAIN_FIRE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: MAIN THRUSTER FIRE. Duration=%u ms.",
                      (unsigned)Msg->Payload.Duration_ms);

    ret = IG4U_Cmd_MainThrusterFire(Msg->Payload.Duration_ms, &result);

    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.MainFireCount++;
        IG4U_Data.CurrentState = IG4U_STATE_STANDBY; /* 분사 완료 → STANDBY */

        /* Fire Result TLM 발행 */
        memcpy(&IG4U_Data.FireResultTlm.Payload, &result, sizeof(result));
        CFE_SB_TimeStampMsg(CFE_MSG_PTR(IG4U_Data.FireResultTlm.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(IG4U_Data.FireResultTlm.TelemetryHeader), true);

        CFE_EVS_SendEvent(IG4U_MAIN_FIRE_DONE_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: MAIN FIRE done. Result=%u ActualDur=%u ms.",
                          result.Result, result.Actual_Duration_ms);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        IG4U_Data.CurrentState = IG4U_STATE_SAFE; /* 오류 시 SAFE */
        CFE_EVS_SendEvent(IG4U_STATE_SAFE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: MAIN FIRE failed. State -> SAFE. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_MainThrusterAbortCmd(const IG4U_MainThrusterAbortCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_MainThrusterAbort();
    IG4U_Data.CurrentState = IG4U_STATE_STANDBY;

    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        CFE_EVS_SendEvent(IG4U_MAIN_ABORT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: MAIN THRUSTER ABORT. State -> STANDBY.");
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_SEND_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: ABORT send failed. RC=%d", (int)ret);
    }
    return CFE_SUCCESS; /* Abort는 전송 실패해도 상태는 복귀 */
}

/* ===================================================================
 * Group 4: 냉가스 추력기
 * =================================================================== */

CFE_Status_t IG4U_CGThrusterPulseCmd(const IG4U_CGThrusterPulseCmd_t *Msg)
{
    int32_t                         ret;
    IG4U_CGThrusterResult_Payload_t result;

    if (!IsArmed())
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_INTERLOCK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: CG PULSE rejected. State=%u.", IG4U_Data.CurrentState);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    memset(&result, 0, sizeof(result));
    IG4U_Data.CurrentState = IG4U_STATE_FIRING;

    CFE_EVS_SendEvent(IG4U_CG_FIRE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: CG PULSE. CH=%u Dur=%u ms.",
                      Msg->Payload.ThrusterID, Msg->Payload.Duration_ms);

    ret = IG4U_Cmd_CGThrusterPulse(Msg->Payload.ThrusterID,
                                    Msg->Payload.Duration_ms,
                                    &result);
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.CGFireCount++;
        IG4U_Data.CurrentState = IG4U_STATE_STANDBY;

        CFE_EVS_SendEvent(IG4U_CG_FIRE_DONE_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: CG PULSE done. CH=%u Result=%u.",
                          result.ThrusterID, result.Result);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        IG4U_Data.CurrentState = IG4U_STATE_SAFE;
        CFE_EVS_SendEvent(IG4U_STATE_SAFE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: CG PULSE failed. State -> SAFE. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_CGThrusterBurstCmd(const IG4U_CGThrusterBurstCmd_t *Msg)
{
    int32_t                         ret;
    IG4U_CGThrusterResult_Payload_t result;

    if (!IsArmed())
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_INTERLOCK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: CG BURST rejected. State=%u.", IG4U_Data.CurrentState);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    memset(&result, 0, sizeof(result));
    IG4U_Data.CurrentState = IG4U_STATE_FIRING;

    CFE_EVS_SendEvent(IG4U_CG_FIRE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: CG BURST. CH=%u Pulse=%u ms x%u Interval=%u ms.",
                      Msg->Payload.ThrusterID, Msg->Payload.PulseDuration_ms,
                      Msg->Payload.PulseCount, Msg->Payload.Interval_ms);

    ret = IG4U_Cmd_CGThrusterBurst(Msg->Payload.ThrusterID,
                                    Msg->Payload.PulseDuration_ms,
                                    Msg->Payload.PulseCount,
                                    Msg->Payload.Interval_ms,
                                    &result);
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.CGFireCount++;
        IG4U_Data.CurrentState = IG4U_STATE_STANDBY;

        CFE_EVS_SendEvent(IG4U_CG_FIRE_DONE_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: CG BURST done. Executed=%u pulses.",
                          result.ExecutedPulseCount);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        IG4U_Data.CurrentState = IG4U_STATE_SAFE;
        CFE_EVS_SendEvent(IG4U_STATE_SAFE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: CG BURST failed. State -> SAFE. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_CGThrusterAbortCmd(const IG4U_CGThrusterAbortCmd_t *Msg)
{
    (void)Msg;

    IG4U_Cmd_CGThrusterAbort();
    IG4U_Data.CmdCounter++;
    IG4U_Data.CurrentState = IG4U_STATE_STANDBY;

    CFE_EVS_SendEvent(IG4U_CG_ABORT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IG4U: CG ABORT. State -> STANDBY.");
    return CFE_SUCCESS;
}

/* ===================================================================
 * Group 5: Fault 관리
 * =================================================================== */

CFE_Status_t IG4U_RequestFaultLogCmd(const IG4U_RequestFaultLogCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_RequestFaultLog(&IG4U_Data.FaultLogTlm.Payload);
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        CFE_SB_TimeStampMsg(CFE_MSG_PTR(IG4U_Data.FaultLogTlm.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(IG4U_Data.FaultLogTlm.TelemetryHeader), true);

        CFE_EVS_SendEvent(IG4U_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: Fault log received. Entries=%u.",
                          IG4U_Data.FaultLogTlm.Payload.EntryCount);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_RECV_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: REQUEST FAULT LOG failed. RC=%d", (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

CFE_Status_t IG4U_ClearFaultCmd(const IG4U_ClearFaultCmd_t *Msg)
{
    int32_t ret;
    (void)Msg;

    ret = IG4U_Cmd_ClearFault();
    if (ret == IG4U_CMD_OK)
    {
        IG4U_Data.CmdCounter++;
        IG4U_Data.FaultFlags   = IG4U_FAULT_NONE;
        IG4U_Data.CurrentState = IG4U_STATE_STANDBY;
        CFE_EVS_SendEvent(IG4U_FAULT_CLEARED_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "IG4U: Fault cleared. State -> STANDBY.");
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_FAULT_CLEAR_FAIL_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: CLEAR FAULT failed (conditions not met). RC=%d",
                          (int)ret);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    return CFE_SUCCESS;
}

/* ===================================================================
 * WAKEUP 핸들러
 * SCH 주기 트리거 → RS-422 HK 폴링 + ARMED 타임아웃 감시
 * =================================================================== */
void IG4U_WakeupHandler(void)
{
    int32_t ret;

    if (!IG4U_Data.PowerApplied || !IG4U_Data.LinkReady)
    {
        return;
    }

    /* FAULT 상태에서는 HK만 시도 (명령 차단) */
    if (!IsCommAllowed() && IG4U_Data.CurrentState != IG4U_STATE_FIRING)
    {
        return;
    }

    /* --- HK 데이터 폴링 --- */
    ret = IG4U_Cmd_RequestHKData(&IG4U_Data.LastHK);
    if (ret == IG4U_CMD_OK)
    {
        /* TLM 갱신 및 발행 */
        IG4U_Data.HkTlm.Payload.CmdCounter    = IG4U_Data.CmdCounter;
        IG4U_Data.HkTlm.Payload.CmdErrCounter = IG4U_Data.ErrCounter;
        IG4U_Data.HkTlm.Payload.Mode          = IG4U_Data.CurrentState;
        IG4U_Data.HkTlm.Payload.FaultFlags    = IG4U_Data.FaultFlags;
        IG4U_Data.HkTlm.Payload.MainFireCount = IG4U_Data.MainFireCount;
        IG4U_Data.HkTlm.Payload.CGFireCount   = IG4U_Data.CGFireCount;
        memcpy(IG4U_Data.HkTlm.Payload.Pressure_kPa_x10,
               IG4U_Data.LastHK.Pressure_kPa_x10,
               sizeof(IG4U_Data.LastHK.Pressure_kPa_x10));
        memcpy(IG4U_Data.HkTlm.Payload.Temperature_x10,
               IG4U_Data.LastHK.Temperature_x10,
               sizeof(IG4U_Data.LastHK.Temperature_x10));

        /* 압력/온도 이상 감지 */
        if (IG4U_Data.LastHK.Pressure_kPa_x10[0] > 5000U ||
            IG4U_Data.LastHK.Pressure_kPa_x10[1] > 5000U)
        {
            CFE_EVS_SendEvent(IG4U_PRESSURE_HIGH_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: Pressure high! P1=%u P2=%u (x0.1kPa)",
                              IG4U_Data.LastHK.Pressure_kPa_x10[0],
                              IG4U_Data.LastHK.Pressure_kPa_x10[1]);
        }

        CFE_SB_TimeStampMsg(CFE_MSG_PTR(IG4U_Data.HkTlm.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(IG4U_Data.HkTlm.TelemetryHeader), true);
    }
    else
    {
        IG4U_Data.ErrCounter++;
        CFE_EVS_SendEvent(IG4U_UART_TIMEOUT_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: Wakeup HK poll failed. RC=%d", (int)ret);
    }

    /* --- ARMED 타임아웃 감시 ---
     * ARMED 상태에서 일정 시간 내 Fire 없으면 자동 STANDBY 복귀
     */
    if (IG4U_Data.ArmedTimerActive && IG4U_Data.CurrentState == IG4U_STATE_ARMED)
    {
        CFE_TIME_SysTime_t now     = CFE_TIME_GetTime();
        CFE_TIME_SysTime_t elapsed = CFE_TIME_Subtract(now, IG4U_Data.ArmedEntryTime);

        if (elapsed.Seconds >= IG4U_ARMED_TIMEOUT_SEC)
        {
            IG4U_Data.CurrentState     = IG4U_STATE_STANDBY;
            IG4U_Data.ArmedTimerActive = false;

            /* 탑재체에도 STANDBY 명령 전송 */
            IG4U_Cmd_SetMode((uint8_t)IG4U_MODE_STANDBY);

            CFE_EVS_SendEvent(IG4U_ARMED_TIMEOUT_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: ARMED timeout (%u sec). Auto -> STANDBY.",
                              IG4U_ARMED_TIMEOUT_SEC);
        }
        else
        {
            IG4U_Data.HkTlm.Payload.ArmTimeout_Remain =
                (uint8_t)(IG4U_ARMED_TIMEOUT_SEC - elapsed.Seconds);
        }
    }
}

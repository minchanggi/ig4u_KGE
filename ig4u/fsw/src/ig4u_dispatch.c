/**
 * @file ig4u_dispatch.c
 * @brief iG4U cFS 메시지 디스패치
 *
 * WAKEUP_MID : 주기적 RS-422 HK 폴링 + ARMED 타임아웃 감시
 * CMD_MID    : 지상 명령 처리
 */

#include "ig4u_task.h"
#include "ig4u_dispatch.h"
#include "ig4u_cmds.h"
#include "ig4u_eventids.h"
#include "ig4u_msgids.h"
#include "ig4u_fcncodes.h"

/* ===================================================================
 * 명령 길이 검증
 * =================================================================== */
bool IG4U_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId   = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(IG4U_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IG4U: Invalid Msg len: ID=0x%X CC=%u Len=%u Exp=%u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                          (unsigned int)FcnCode,
                          (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;
        IG4U_Data.ErrCounter++;
    }

    return result;
}

/* ===================================================================
 * 지상 명령 처리 (CMD_MID)
 * Function Code → 핸들러 라우팅
 * =================================================================== */
void IG4U_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CC = 0xFF;
    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CC);

    switch (CC)
    {
        /* 시스템 */
        case IG4U_NOOP_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_NoopCmd_t)))
                IG4U_NoopCmd((const IG4U_NoopCmd_t *)SBBufPtr);
            break;

        case IG4U_RESET_COUNTER_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_ResetCounterCmd_t)))
                IG4U_ResetCounterCmd((const IG4U_ResetCounterCmd_t *)SBBufPtr);
            break;

        /* Group 1: 모듈 제어 */
        case IG4U_PING_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_PingCmd_t)))
                IG4U_PingCmd((const IG4U_PingCmd_t *)SBBufPtr);
            break;

        case IG4U_RESET_MODULE_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_ResetModuleCmd_t)))
                IG4U_ResetModuleCmd((const IG4U_ResetModuleCmd_t *)SBBufPtr);
            break;

        case IG4U_SET_MODE_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_SetModeCmd_t)))
                IG4U_SetModeCmd((const IG4U_SetModeCmd_t *)SBBufPtr);
            break;

        case IG4U_ARM_PROPULSION_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_ArmPropulsionCmd_t)))
                IG4U_ArmPropulsionCmd((const IG4U_ArmPropulsionCmd_t *)SBBufPtr);
            break;

        case IG4U_DISARM_PROPULSION_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_DisarmPropulsionCmd_t)))
                IG4U_DisarmPropulsionCmd((const IG4U_DisarmPropulsionCmd_t *)SBBufPtr);
            break;

        /* Group 2: HK / 상태 */
        case IG4U_REQUEST_HK_DATA_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_RequestHKDataCmd_t)))
                IG4U_RequestHKDataCmd((const IG4U_RequestHKDataCmd_t *)SBBufPtr);
            break;

        case IG4U_REQUEST_STATUS_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_RequestStatusCmd_t)))
                IG4U_RequestStatusCmd((const IG4U_RequestStatusCmd_t *)SBBufPtr);
            break;

        /* Group 3: 주추력기 */
        case IG4U_MAIN_THRUSTER_FIRE_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_MainThrusterFireCmd_t)))
                IG4U_MainThrusterFireCmd((const IG4U_MainThrusterFireCmd_t *)SBBufPtr);
            break;

        case IG4U_MAIN_THRUSTER_ABORT_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_MainThrusterAbortCmd_t)))
                IG4U_MainThrusterAbortCmd((const IG4U_MainThrusterAbortCmd_t *)SBBufPtr);
            break;

        /* Group 4: 냉가스 추력기 */
        case IG4U_CG_THRUSTER_PULSE_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_CGThrusterPulseCmd_t)))
                IG4U_CGThrusterPulseCmd((const IG4U_CGThrusterPulseCmd_t *)SBBufPtr);
            break;

        case IG4U_CG_THRUSTER_BURST_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_CGThrusterBurstCmd_t)))
                IG4U_CGThrusterBurstCmd((const IG4U_CGThrusterBurstCmd_t *)SBBufPtr);
            break;

        case IG4U_CG_THRUSTER_ABORT_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_CGThrusterAbortCmd_t)))
                IG4U_CGThrusterAbortCmd((const IG4U_CGThrusterAbortCmd_t *)SBBufPtr);
            break;

        /* Group 5: Fault 관리 */
        case IG4U_REQUEST_FAULT_LOG_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_RequestFaultLogCmd_t)))
                IG4U_RequestFaultLogCmd((const IG4U_RequestFaultLogCmd_t *)SBBufPtr);
            break;

        case IG4U_CLEAR_FAULT_CC:
            if (IG4U_VerifyCmdLength(&SBBufPtr->Msg, sizeof(IG4U_ClearFaultCmd_t)))
                IG4U_ClearFaultCmd((const IG4U_ClearFaultCmd_t *)SBBufPtr);
            break;

        default:
            IG4U_Data.ErrCounter++;
            CFE_EVS_SendEvent(IG4U_CC_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: Invalid CC=%d", (int)CC);
            break;
    }
}

/* ===================================================================
 * 메인 파이프 핸들러
 * MsgId → 핸들러 라우팅
 * =================================================================== */
void IG4U_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case IG4U_CMD_MID:
            IG4U_ProcessGroundCommand(SBBufPtr);
            break;

        case IG4U_WAKEUP_MID:
            IG4U_WakeupHandler();
            break;

        default:
            CFE_EVS_SendEvent(IG4U_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IG4U: Invalid MID=0x%X",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}

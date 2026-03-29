/**
 * @file ig4u_cmds.h
 * @brief iG4U cFS 지상 명령 핸들러 및 RS-422 통신 유틸리티
 *
 * dispatch.c에서 호출하는 cFS 레이어 핸들러.
 * 각 핸들러는 내부적으로 ig4u_commands.c의 Wire-level TC 함수를 호출.
 */
#ifndef IG4U_CMDS_H
#define IG4U_CMDS_H

#include "cfe.h"
#include "ig4u_msg.h"

/* ------------------------------------------------------------------
 * 시스템 명령 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_NoopCmd(const IG4U_NoopCmd_t *Msg);
CFE_Status_t IG4U_ResetCounterCmd(const IG4U_ResetCounterCmd_t *Msg);

/* ------------------------------------------------------------------
 * Group 1: 모듈 제어 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_PingCmd(const IG4U_PingCmd_t *Msg);
CFE_Status_t IG4U_ResetModuleCmd(const IG4U_ResetModuleCmd_t *Msg);
CFE_Status_t IG4U_SetModeCmd(const IG4U_SetModeCmd_t *Msg);
CFE_Status_t IG4U_ArmPropulsionCmd(const IG4U_ArmPropulsionCmd_t *Msg);
CFE_Status_t IG4U_DisarmPropulsionCmd(const IG4U_DisarmPropulsionCmd_t *Msg);

/* ------------------------------------------------------------------
 * Group 2: HK / 상태 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_RequestHKDataCmd(const IG4U_RequestHKDataCmd_t *Msg);
CFE_Status_t IG4U_RequestStatusCmd(const IG4U_RequestStatusCmd_t *Msg);

/* ------------------------------------------------------------------
 * Group 3: 주추력기 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_MainThrusterFireCmd(const IG4U_MainThrusterFireCmd_t *Msg);
CFE_Status_t IG4U_MainThrusterAbortCmd(const IG4U_MainThrusterAbortCmd_t *Msg);

/* ------------------------------------------------------------------
 * Group 4: 냉가스 추력기 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_CGThrusterPulseCmd(const IG4U_CGThrusterPulseCmd_t *Msg);
CFE_Status_t IG4U_CGThrusterBurstCmd(const IG4U_CGThrusterBurstCmd_t *Msg);
CFE_Status_t IG4U_CGThrusterAbortCmd(const IG4U_CGThrusterAbortCmd_t *Msg);

/* ------------------------------------------------------------------
 * Group 5: Fault 관리 핸들러
 * ------------------------------------------------------------------ */
CFE_Status_t IG4U_RequestFaultLogCmd(const IG4U_RequestFaultLogCmd_t *Msg);
CFE_Status_t IG4U_ClearFaultCmd(const IG4U_ClearFaultCmd_t *Msg);

/* ------------------------------------------------------------------
 * WAKEUP 핸들러 (dispatch에서 직접 호출)
 * SCH 주기 트리거 → RS-422 HK 폴링 + ARMED 타임아웃 감시
 * ------------------------------------------------------------------ */
void IG4U_WakeupHandler(void);

#endif /* IG4U_CMDS_H */

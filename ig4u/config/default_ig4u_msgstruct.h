#ifndef IG4U_MSGSTRUCT_H
#define IG4U_MSGSTRUCT_H

/**
 * @file ig4u_msgstruct.h
 * @brief iG4U cFS 전체 메시지 구조체 (CFE_MSG 헤더 포함)
 */

#include "ig4u_mission_cfg.h"
#include "ig4u_msgdefs.h"
#include "cfe.h"

/* ------------------------------------------------------------------
 * Telemetry Messages (OBC → Ground)
 * ------------------------------------------------------------------ */

/** HK 데이터 텔레메트리 */
typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    IG4U_HkTlm_Payload_t      Payload;
} IG4U_HkTlm_t;

/** 상태 텔레메트리 */
typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    IG4U_ReplyStatus_Payload_t Payload;
} IG4U_StatusTlm_t;

/** 주추력기 분사 결과 텔레메트리 */
typedef struct {
    CFE_MSG_TelemetryHeader_t       TelemetryHeader;
    IG4U_MainThrusterResult_Payload_t Payload;
} IG4U_MainFireResultTlm_t;

/** 냉가스 분사 결과 텔레메트리 */
typedef struct {
    CFE_MSG_TelemetryHeader_t     TelemetryHeader;
    IG4U_CGThrusterResult_Payload_t Payload;
} IG4U_CGFireResultTlm_t;

/** Fault 로그 텔레메트리 */
typedef struct {
    CFE_MSG_TelemetryHeader_t   TelemetryHeader;
    IG4U_ReplyFaultLog_Payload_t Payload;
} IG4U_FaultLogTlm_t;

/* ------------------------------------------------------------------
 * Command Messages (Ground → OBC → iG4U)
 * ------------------------------------------------------------------ */

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_NoopCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_ResetCounterCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_PingCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_ResetModuleCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t  CommandHeader;
    IG4U_SetMode_Payload_t   Payload;
} IG4U_SetModeCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_ArmPropulsionCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_DisarmPropulsionCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_RequestHKDataCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_RequestStatusCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t           CommandHeader;
    IG4U_MainThrusterFire_Payload_t   Payload;
} IG4U_MainThrusterFireCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_MainThrusterAbortCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t         CommandHeader;
    IG4U_CGThrusterPulse_Payload_t  Payload;
} IG4U_CGThrusterPulseCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t         CommandHeader;
    IG4U_CGThrusterBurst_Payload_t  Payload;
} IG4U_CGThrusterBurstCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_CGThrusterAbortCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_RequestFaultLogCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} IG4U_ClearFaultCmd_t;

#endif /* IG4U_MSGSTRUCT_H */

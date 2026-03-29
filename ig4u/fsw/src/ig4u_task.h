/**
 * @file ig4u_task.h
 * @brief iG4U 추력 유닛 cFS 메인 태스크 헤더
 *
 * PAYLGBAT 패턴 준수: child task 없음.
 * SCH가 WAKEUP_MID를 주기적으로 전송 → 메인 태스크 파이프에서
 * RS-422 HK 폴링 및 명령 처리 수행.
 */
#ifndef IG4U_TASK_H
#define IG4U_TASK_H

#include "cfe.h"
#include "ig4u_mission_cfg.h"
#include "ig4u_perfids.h"
#include "ig4u_msgids.h"
#include "ig4u_msg.h"
#include "ig4u_internal_cfg.h"

/* ===================================================================
 * 앱 전역 데이터 구조체
 * =================================================================== */
typedef struct
{
    /* 앱 상태 */
    uint8_t  CmdCounter;
    uint8_t  ErrCounter;
    uint32_t RunStatus;

    /* Software Bus */
    CFE_SB_PipeId_t CmdPipe;
    char            CmdPipeName[CFE_MISSION_MAX_API_LEN];
    uint16_t        PipeDepth;

    /* Telemetry 메시지 캐시 */
    IG4U_HkTlm_t          HkTlm;
    IG4U_StatusTlm_t      StatusTlm;
    IG4U_MainFireResultTlm_t FireResultTlm;
    IG4U_FaultLogTlm_t    FaultLogTlm;

    /* iG4U 운용 상태 */
    IG4U_State_t    CurrentState;       /**< 현재 상태머신 상태              */
    uint16_t        FaultFlags;         /**< 누적 Fault 플래그               */
    bool            PowerApplied;       /**< 탑재체 전원 인가 여부           */
    bool            LinkReady;          /**< RS-422 링크 정상 여부           */

    /* HK 캐시 (WAKEUP 시 갱신) */
    IG4U_ReplyHKData_Payload_t    LastHK;
    IG4U_ReplyStatus_Payload_t    LastStatus;

    /* 분사 통계 */
    uint16_t MainFireCount;             /**< 주추력기 분사 횟수              */
    uint16_t CGFireCount;               /**< 냉가스 추력기 분사 횟수         */

    /* ARMED 타임아웃 관리 */
    CFE_TIME_SysTime_t ArmedEntryTime;  /**< ARMED 진입 시각                 */
    bool               ArmedTimerActive;/**< ARMED 타임아웃 카운트 중        */

} IG4U_Data_t;

extern IG4U_Data_t IG4U_Data;

/* ===================================================================
 * API
 * =================================================================== */
void         IG4U_Main(void);
CFE_Status_t IG4U_Init(void);

#endif /* IG4U_TASK_H */

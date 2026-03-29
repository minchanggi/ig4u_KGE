#ifndef IG4U_COMMANDS_H
#define IG4U_COMMANDS_H

/**
 * @file ig4u_commands.h
 * @brief iG4U TC 명령 빌더 및 TM 응답 파서 API
 *
 * 상위 레이어(cFS App)에서 호출하는 인터페이스입니다.
 * 각 함수는 내부적으로 IG4U_Proto_EncodeTC()를 사용하여
 * RS-422 송신 패킷을 생성하고 UART를 통해 전송합니다.
 *
 * 참조: IG4U-SW-ICD-001 Section 4.2, 6.1
 */

#include "common_types.h"
#include "ig4u_msgdefs.h"

/* 반환 코드 */
#define IG4U_CMD_OK            0
#define IG4U_CMD_ERR_ENCODE   -1
#define IG4U_CMD_ERR_SEND     -2
#define IG4U_CMD_ERR_TIMEOUT  -3
#define IG4U_CMD_ERR_NACK     -4
#define IG4U_CMD_ERR_PARSE    -5

/* ===================================================================
 * Group 1: 모듈 제어
 * =================================================================== */

/** 통신 상태 확인 - MsgID=1, 응답: Default Response */
int32_t IG4U_Cmd_Ping(void);

/** 추진모듈 소프트 리셋 - MsgID=2, 응답: Default Response */
int32_t IG4U_Cmd_ResetModule(void);

/**
 * 모듈 운용 모드 설정 - MsgID=3, 응답: Default Response
 * @param mode IG4U_ModeCmd_t (STANDBY=0, ARMED=1, SAFE=2)
 */
int32_t IG4U_Cmd_SetMode(uint8_t mode);

/** 추진계 Arm - MsgID=4, 응답: Default Response (인터록 체크 포함) */
int32_t IG4U_Cmd_ArmPropulsion(void);

/** 추진계 Disarm - MsgID=5, 응답: Default Response */
int32_t IG4U_Cmd_DisarmPropulsion(void);

/* ===================================================================
 * Group 2: HK / 상태
 * =================================================================== */

/**
 * HK 데이터 요청 - MsgID=10
 * @param out_hk [출력] 수신된 HK 데이터 구조체
 */
int32_t IG4U_Cmd_RequestHKData(IG4U_ReplyHKData_Payload_t *out_hk);

/**
 * 상태 정보 요청 - MsgID=11
 * @param out_status [출력] 수신된 상태 구조체
 */
int32_t IG4U_Cmd_RequestStatus(IG4U_ReplyStatus_Payload_t *out_status);

/* ===================================================================
 * Group 3: 주추력기
 * =================================================================== */

/**
 * 1N 주추력기 분사 - MsgID=20
 * @param duration_ms 분사 시간 (ms). 0 = Abort 까지 연속
 * @param out_result  [출력] 분사 결과 (NULL 가능, 결과 대기 생략)
 */
int32_t IG4U_Cmd_MainThrusterFire(uint32_t                          duration_ms,
                                   IG4U_MainThrusterResult_Payload_t *out_result);

/** 주추력기 즉시 중단 - MsgID=21 */
int32_t IG4U_Cmd_MainThrusterAbort(void);

/* ===================================================================
 * Group 4: 냉가스 추력기
 * =================================================================== */

/**
 * 냉가스 단일 펄스 - MsgID=30
 * @param thruster_id  채널 ID (0~IG4U_CG_THRUSTER_COUNT-1)
 * @param duration_ms  펄스 시간 (ms)
 * @param out_result   [출력] 분사 결과 (NULL 가능)
 */
int32_t IG4U_Cmd_CGThrusterPulse(uint8_t                         thruster_id,
                                   uint16_t                        duration_ms,
                                   IG4U_CGThrusterResult_Payload_t *out_result);

/**
 * 냉가스 반복 펄스 - MsgID=31
 * @param thruster_id      채널 ID
 * @param pulse_duration_ms 단일 펄스 시간 (ms)
 * @param pulse_count       반복 횟수
 * @param interval_ms       펄스 간 간격 (ms)
 * @param out_result        [출력] 분사 결과 (NULL 가능)
 */
int32_t IG4U_Cmd_CGThrusterBurst(uint8_t                         thruster_id,
                                   uint16_t                        pulse_duration_ms,
                                   uint16_t                        pulse_count,
                                   uint16_t                        interval_ms,
                                   IG4U_CGThrusterResult_Payload_t *out_result);

/** 냉가스 추력기 즉시 중단 - MsgID=32 */
int32_t IG4U_Cmd_CGThrusterAbort(void);

/* ===================================================================
 * Group 5: Fault 관리
 * =================================================================== */

/**
 * Fault 로그 요청 - MsgID=40
 * @param out_log [출력] Fault 로그 데이터
 */
int32_t IG4U_Cmd_RequestFaultLog(IG4U_ReplyFaultLog_Payload_t *out_log);

/**
 * Fault 상태 해제 시도 - MsgID=41
 * SAFE 상태에서 조건 충족 시 STANDBY 복귀
 */
int32_t IG4U_Cmd_ClearFault(void);

/* ===================================================================
 * 내부 유틸리티 (ig4u_commands.c 내부 사용, 테스트용으로 노출)
 * =================================================================== */

/**
 * Default Response 수신 대기 및 파싱
 * @param expected_msg_id  예상 MsgID
 * @param out_resp         [출력] 응답 내용 (NULL 가능)
 */
int32_t IG4U_Cmd_WaitDefaultResponse(uint8_t                         expected_msg_id,
                                       IG4U_DefaultResponse_Payload_t *out_resp);

#endif /* IG4U_COMMANDS_H */

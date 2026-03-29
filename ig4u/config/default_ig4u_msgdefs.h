#ifndef IG4U_MSGDEFS_H
#define IG4U_MSGDEFS_H

/**
 * @file ig4u_msgdefs.h
 * @brief iG4U 추력 유닛 메시지 페이로드 구조체 정의
 *
 * 참조: IG4U-SW-ICD-001 Section 4 (탑재체 메시지 상세 정의)
 *
 * NOTE: 이 파일은 RS-422 직렬 링크를 통해 실제로 송수신되는
 *       페이로드(Payload) 부분의 구조를 정의합니다.
 *       Section 4.3 상세 내용이 TBD이므로, ICD 기술 내용과 운용 특성을
 *       기반으로 합리적인 구조를 정의하였습니다. 실제 ICD 확정 후 수정 필요.
 *
 * 패킷 전체 구조 (ICD Section 4.1 표 1):
 *   [0-1]  Header   : 0xAA55
 *   [2]    Version  : Major(7:4) | Minor(3:0)
 *   [3]    MsgType  : 0=TC, 1=TM(Response), 2=Event
 *   [4]    MsgID    : 메시지 고유 ID
 *   [5-6]  Length   : Payload 길이 (bytes)
 *   [7..N] Payload  : 아래 구조체들
 *   [N+1..N+2] CRC16
 */

#include "common_types.h"
#include "ig4u_fcncodes.h"

/* ===================================================================
 * 공통: Default Response (TM)
 * 특정 응답이 정의되지 않은 TC 수신 시 ACK/NACK 반환
 * MsgID = 수신한 TC의 MsgID와 동일
 * =================================================================== */
typedef struct {
    uint8_t  Result;         /**< 0=ACK, 1=NACK                         */
    uint8_t  EchoMsgID;      /**< 원본 TC의 MsgID                       */
    uint8_t  ErrorCode;      /**< NACK 세부 원인 (ig4u_internal_cfg.h)  */
    uint8_t  Spare;          /**< 예약 (4바이트 정렬)                   */
} __attribute__((packed)) IG4U_DefaultResponse_Payload_t;

/* ===================================================================
 * Group 1: 모듈 제어 메시지
 * =================================================================== */

/* Ping TC (MsgID=1) - Payload 없음, Length=0 */

/* Reset Module TC (MsgID=2) - Payload 없음, Length=0 */

/**
 * Set Mode TC (MsgID=3)
 * 모드: 0=STANDBY, 1=ARMED, 2=SAFE (ig4u_internal_cfg.h IG4U_ModeCmd_t)
 */
typedef struct {
    uint8_t  Mode;           /**< 설정할 모드 코드                      */
    uint8_t  Spare[3];       /**< 예약 (4바이트 정렬)                   */
} __attribute__((packed)) IG4U_SetMode_Payload_t;

/* Arm Propulsion TC (MsgID=4) - Payload 없음, Length=0 */
/* Disarm Propulsion TC (MsgID=5) - Payload 없음, Length=0 */

/* ===================================================================
 * Group 2: HK / Status 메시지
 * =================================================================== */

/* Request HK Data TC (MsgID=10) - Payload 없음, Length=0 */

/**
 * Reply HK Data TM (MsgID=10)
 * ICD: 압력 2채널 + 온도 4채널 + 상태정보
 * 압력 단위: [TBD] kPa × 10 (0.1 kPa LSB)
 * 온도 단위: [TBD] °C × 10 (0.1°C LSB), int16 signed
 */
typedef struct {
    uint16_t Pressure_kPa_x10[2];   /**< 압력 CH1~CH2 (0.1 kPa/LSB)   */
    int16_t  Temperature_x10[4];    /**< 온도 CH1~CH4 (0.1°C/LSB)      */
    uint8_t  Mode;                  /**< 현재 상태 (IG4U_State_t)       */
    uint8_t  FaultFlags_Lo;         /**< Fault 플래그 하위 8비트        */
    uint8_t  FaultFlags_Hi;         /**< Fault 플래그 상위 8비트        */
    uint8_t  Spare;
} __attribute__((packed)) IG4U_ReplyHKData_Payload_t;

/* Request Status TC (MsgID=11) - Payload 없음, Length=0 */

/**
 * Reply Status TM (MsgID=11)
 */
typedef struct {
    uint8_t  Mode;               /**< 현재 상태 (IG4U_State_t)          */
    uint8_t  InterlockStatus;    /**< 인터록 상태 비트맵 (0=OK)         */
    uint16_t FaultFlags;         /**< Fault 플래그 비트맵               */
    uint8_t  ArmTimeout_Remain;  /**< ARMED 타임아웃 잔여 시간 (초)     */
    uint8_t  Spare[3];
} __attribute__((packed)) IG4U_ReplyStatus_Payload_t;

/* Interlock 비트 정의 (InterlockStatus) */
#define IG4U_INTERLOCK_POWER_OK     (1U << 0)  /**< 전원 인가 정상     */
#define IG4U_INTERLOCK_PRESSURE_OK  (1U << 1)  /**< 탱크 압력 정상     */
#define IG4U_INTERLOCK_TEMP_OK      (1U << 2)  /**< 온도 정상          */
#define IG4U_INTERLOCK_VALVE_OK     (1U << 3)  /**< 밸브 상태 정상     */

/* ===================================================================
 * Group 3: 주추력기 메시지
 * =================================================================== */

/**
 * Main Thruster Fire TC (MsgID=20)
 * 1N 주추력기 분사 명령
 */
typedef struct {
    uint32_t Duration_ms;    /**< 분사 지속 시간 (ms). 0=연속(Abort 시까지) */
} __attribute__((packed)) IG4U_MainThrusterFire_Payload_t;

/* Main Thruster Abort TC (MsgID=21) - Payload 없음, Length=0 */

/**
 * Main Thruster Result TM (MsgID=20)
 * 분사 완료 후 결과 반환
 */
typedef struct {
    uint8_t  Result;             /**< 0=정상완료, 1=Abort, 2=Fault중단  */
    uint8_t  Spare;
    uint16_t Actual_Duration_ms; /**< 실제 분사 시간 (ms)               */
    uint16_t FaultFlags;         /**< 분사 중 발생한 Fault 플래그       */
    uint8_t  Spare2[2];
} __attribute__((packed)) IG4U_MainThrusterResult_Payload_t;

/* Result 코드 정의 */
#define IG4U_FIRE_RESULT_OK         0U
#define IG4U_FIRE_RESULT_ABORTED    1U
#define IG4U_FIRE_RESULT_FAULT      2U

/* ===================================================================
 * Group 4: 냉가스 추력기 메시지
 * =================================================================== */

/**
 * CG Thruster Pulse TC (MsgID=30)
 * 냉가스 추력기 단일 펄스
 */
typedef struct {
    uint8_t  ThrusterID;         /**< 추력기 채널 ID (0~N-1)            */
    uint8_t  Spare;
    uint16_t Duration_ms;        /**< 펄스 지속 시간 (ms)               */
} __attribute__((packed)) IG4U_CGThrusterPulse_Payload_t;

/**
 * CG Thruster Burst TC (MsgID=31)
 * 냉가스 추력기 반복 펄스
 */
typedef struct {
    uint8_t  ThrusterID;         /**< 추력기 채널 ID                    */
    uint8_t  Spare;
    uint16_t PulseDuration_ms;   /**< 단일 펄스 시간 (ms)               */
    uint16_t PulseCount;         /**< 반복 횟수                         */
    uint16_t Interval_ms;        /**< 펄스 간 간격 (ms)                 */
} __attribute__((packed)) IG4U_CGThrusterBurst_Payload_t;

/* CG Thruster Abort TC (MsgID=32) - Payload 없음, Length=0 */

/**
 * CG Thruster Result TM (MsgID = triggering TC MsgID: 30~32)
 * 냉가스 분사 결과
 */
typedef struct {
    uint8_t  TriggerMsgID;       /**< 원인 TC MsgID (30/31/32)          */
    uint8_t  ThrusterID;         /**< 분사한 채널 ID                    */
    uint8_t  Result;             /**< 0=정상, 1=Abort, 2=Fault          */
    uint8_t  Spare;
    uint16_t ExecutedPulseCount; /**< 실제 수행된 펄스 횟수             */
    uint16_t FaultFlags;         /**< 발생한 Fault 플래그               */
} __attribute__((packed)) IG4U_CGThrusterResult_Payload_t;

/* ===================================================================
 * Group 5: Fault 관리 메시지
 * =================================================================== */

/* Request Fault Log TC (MsgID=40) - Payload 없음, Length=0 */

/**
 * Fault Log 단일 항목
 */
typedef struct {
    uint32_t Timestamp_sec;      /**< 발생 시각 (미션 경과 초)          */
    uint16_t FaultCode;          /**< Fault 종류 코드                   */
    uint8_t  State_at_Fault;     /**< 발생 당시 상태 (IG4U_State_t)     */
    uint8_t  Severity;           /**< 0=Warning, 1=Fault, 2=Critical    */
} __attribute__((packed)) IG4U_FaultEntry_t;

/**
 * Reply Fault Log TM (MsgID=40)
 * Fault 기록 반환 (최대 IG4U_FAULT_LOG_MAX_ENTRIES 항목)
 */
typedef struct {
    uint8_t         EntryCount;  /**< 유효한 Fault 항목 수              */
    uint8_t         Spare[3];
    IG4U_FaultEntry_t Entries[32]; /**< Fault 항목 배열 (32항목 한도)   */
} __attribute__((packed)) IG4U_ReplyFaultLog_Payload_t;

/**
 * Clear Fault TC (MsgID=41) - Payload 없음, Length=0
 * SAFE 상태 해제 시도. 조건 충족 시 STANDBY 복귀
 */

/* ===================================================================
 * cFS TLM용 HK 요약 (OBC 내부 telemetry)
 * =================================================================== */
typedef struct {
    uint8_t  CmdCounter;
    uint8_t  CmdErrCounter;

    /* 현재 상태 */
    uint8_t  Mode;               /**< IG4U_State_t                      */
    uint8_t  InterlockStatus;

    /* HK 데이터 */
    uint16_t Pressure_kPa_x10[2];
    int16_t  Temperature_x10[4];

    /* Fault */
    uint16_t FaultFlags;

    /* 분사 통계 */
    uint16_t MainFireCount;      /**< 주추력기 분사 횟수                */
    uint16_t CGFireCount;        /**< 냉가스 분사 횟수                  */

    uint8_t  ArmTimeout_Remain;
    uint8_t  Spare[3];
} __attribute__((packed)) IG4U_HkTlm_Payload_t;

#endif /* IG4U_MSGDEFS_H */

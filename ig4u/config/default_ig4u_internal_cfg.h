#ifndef IG4U_INTERNAL_CFG_H
#define IG4U_INTERNAL_CFG_H

/**
 * @file ig4u_internal_cfg.h
 * @brief iG4U 내부 설정 - 디버그, 상태 머신 정의
 *
 * 참조: IG4U-SW-ICD-001 Section 5 (상태 머신 정의)
 */

/* ------------------------------------------------------------------
 * Debug 설정
 * ------------------------------------------------------------------ */
#define IG4U_DEBUG
/* #undef IG4U_DEBUG */

#ifdef IG4U_DEBUG
#define IG4U_PRINTF(...)   OS_printf(__VA_ARGS__)
#else
#define IG4U_PRINTF(...)
#endif

/* ------------------------------------------------------------------
 * SB Pipe 설정
 * ------------------------------------------------------------------ */
#define IG4U_PIPE_DEPTH     8

/* ------------------------------------------------------------------
 * 상태 머신 (State Machine)
 * ICD Section 5.1: iG4U 상태 머신
 * ------------------------------------------------------------------ */

/**
 * @brief iG4U 운용 상태 정의
 *
 * BOOT     : 전원 인가 직후 초기 부팅 상태. MCU/통신/센서/변수 초기화 및 BIT 수행
 * STANDBY  : 기본 대기 상태. 통신/HK/Status 응답 가능. 추력기 구동 불가. ARM 명령만 허용
 * ARMED    : 추진계 구동 준비 상태. 추력기 명령 수신 가능.
 *            일정 시간 내 실행 없으면 자동 STANDBY 복귀
 * FIRING   : 추력기 구동 상태 (주추력기 또는 냉가스). Abort 명령 허용.
 *            내부 Fault 발생 시 즉시 SAFE 또는 FAULT 전이
 * SAFE     : 보호 상태. 모든 밸브 닫힘, 점화기 OFF.
 *            위험 명령 차단. HK/Status 응답만 허용.
 *            Clear Fault 또는 Set Mode(STANDBY) 조건 충족 시 복귀
 * FAULT    : 중대한 이상 상태. 센서/통신/구동 치명적 오류.
 *            자동 복귀 없이 유지. Reset 또는 지상 승인 절차 필요
 */
typedef enum {
    IG4U_STATE_BOOT    = 0,
    IG4U_STATE_STANDBY = 1,
    IG4U_STATE_ARMED   = 2,
    IG4U_STATE_FIRING  = 3,
    IG4U_STATE_SAFE    = 4,
    IG4U_STATE_FAULT   = 5,
    IG4U_STATE_COUNT           /**< 상태 수 (범위 검사용) */
} IG4U_State_t;

/* Set Mode TC에서 사용하는 모드 코드 */
typedef enum {
    IG4U_MODE_STANDBY = 0,
    IG4U_MODE_ARMED   = 1,
    IG4U_MODE_SAFE    = 2,
} IG4U_ModeCmd_t;

/* ------------------------------------------------------------------
 * Fault 플래그 비트 정의
 * ------------------------------------------------------------------ */
#define IG4U_FAULT_NONE             0x0000U
#define IG4U_FAULT_COMM_ERROR       (1U << 0)   /**< 통신 오류              */
#define IG4U_FAULT_SENSOR_ERROR     (1U << 1)   /**< 센서 이상              */
#define IG4U_FAULT_PRESSURE_HIGH    (1U << 2)   /**< 압력 초과              */
#define IG4U_FAULT_TEMP_HIGH        (1U << 3)   /**< 온도 초과              */
#define IG4U_FAULT_VALVE_ERROR      (1U << 4)   /**< 밸브 구동 오류         */
#define IG4U_FAULT_INTERLOCK        (1U << 5)   /**< 인터록 조건 불만족     */
#define IG4U_FAULT_CRC_ERROR        (1U << 6)   /**< CRC 오류               */
#define IG4U_FAULT_TIMEOUT          (1U << 7)   /**< 응답 타임아웃          */

/* ------------------------------------------------------------------
 * ACK / NACK 코드 (Default Response)
 * ------------------------------------------------------------------ */
#define IG4U_RESULT_ACK             0U
#define IG4U_RESULT_NACK            1U

/* NACK 세부 원인 코드 */
#define IG4U_ERR_NONE               0x00U
#define IG4U_ERR_INVALID_CMD        0x01U   /**< 알 수 없는 명령 ID        */
#define IG4U_ERR_INVALID_STATE      0x02U   /**< 현재 상태에서 불허 명령   */
#define IG4U_ERR_CRC_FAIL           0x03U   /**< CRC 검증 실패             */
#define IG4U_ERR_INTERLOCK_FAIL     0x04U   /**< 인터록 조건 미충족        */
#define IG4U_ERR_PARAM_OUT_OF_RANGE 0x05U   /**< 파라미터 범위 초과        */
#define IG4U_ERR_HARDWARE_FAULT     0x06U   /**< 하드웨어 이상             */

/* ------------------------------------------------------------------
 * 냉가스 추력기 채널 정의 (TBD: 실제 채널 수 확인 필요)
 * ------------------------------------------------------------------ */
#define IG4U_CG_THRUSTER_COUNT      4U      /**< [TBD] 냉가스 추력기 수    */

#endif /* IG4U_INTERNAL_CFG_H */

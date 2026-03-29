#ifndef IG4U_FCNCODES_H
#define IG4U_FCNCODES_H

/**
 * @file ig4u_fcncodes.h
 * @brief iG4U 추력 유닛 Ground Command Function Codes
 *
 * 참조: IG4U-SW-ICD-001 Section 4.2 (표 2 고유 메시지 구현)
 */

/* ------------------------------------------------------------------
 * Group 0: Basic / System Commands
 * ------------------------------------------------------------------ */
#define IG4U_NOOP_CC                0   /**< No-operation                      */
#define IG4U_RESET_COUNTER_CC       1   /**< Reset command/error counters      */

/* ------------------------------------------------------------------
 * Group 1: Module Control Commands
 * ------------------------------------------------------------------ */
#define IG4U_PING_CC                10  /**< 통신 상태 확인 (MsgID=1)          */
#define IG4U_RESET_MODULE_CC        11  /**< 추진모듈 소프트 리셋 (MsgID=2)    */
#define IG4U_SET_MODE_CC            12  /**< 운용 모드 설정 (MsgID=3)          */
#define IG4U_ARM_PROPULSION_CC      13  /**< 추진계 Arm (MsgID=4)              */
#define IG4U_DISARM_PROPULSION_CC   14  /**< 추진계 Disarm (MsgID=5)           */

/* ------------------------------------------------------------------
 * Group 2: Housekeeping / Status Commands
 * ------------------------------------------------------------------ */
#define IG4U_REQUEST_HK_DATA_CC     20  /**< HK 데이터 요청 (MsgID=10)         */
#define IG4U_REQUEST_STATUS_CC      21  /**< 상태 정보 요청 (MsgID=11)         */

/* ------------------------------------------------------------------
 * Group 3: Main Thruster Commands
 * ------------------------------------------------------------------ */
#define IG4U_MAIN_THRUSTER_FIRE_CC  30  /**< 1N 주추력기 분사 (MsgID=20)       */
#define IG4U_MAIN_THRUSTER_ABORT_CC 31  /**< 주추력기 즉시 중단 (MsgID=21)     */

/* ------------------------------------------------------------------
 * Group 4: Cold Gas Thruster Commands
 * ------------------------------------------------------------------ */
#define IG4U_CG_THRUSTER_PULSE_CC   40  /**< 냉가스 단일 펄스 (MsgID=30)       */
#define IG4U_CG_THRUSTER_BURST_CC   41  /**< 냉가스 반복 펄스 (MsgID=31)       */
#define IG4U_CG_THRUSTER_ABORT_CC   42  /**< 냉가스 중단 (MsgID=32)            */

/* ------------------------------------------------------------------
 * Group 5: Fault Management Commands
 * ------------------------------------------------------------------ */
#define IG4U_REQUEST_FAULT_LOG_CC   50  /**< Fault 로그 요청 (MsgID=40)        */
#define IG4U_CLEAR_FAULT_CC         51  /**< Fault 상태 해제 (MsgID=41)        */

#endif /* IG4U_FCNCODES_H */

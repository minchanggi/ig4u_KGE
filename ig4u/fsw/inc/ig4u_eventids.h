#ifndef IG4U_EVENTIDS_H
#define IG4U_EVENTIDS_H

/**
 * @file ig4u_eventids.h
 * @brief iG4U 추력 유닛 cFS Event IDs
 *
 * 참조: IG4U-SW-ICD-001 Section 5 (상태 머신), Section 6 (통신 시험)
 * 구조: PAYLGBAT eventids.h 패턴 준수
 */

/* ------------------------------------------------------------------
 * 0~19: 시스템 / 앱 초기화
 * ------------------------------------------------------------------ */
#define IG4U_RESERVED_EID           0
#define IG4U_INIT_INF_EID           1   /**< 앱 초기화 완료                    */
#define IG4U_CC_ERR_EID             2   /**< 알 수 없는 Command Code           */
#define IG4U_MID_ERR_EID            3   /**< 알 수 없는 Message ID             */
#define IG4U_CMD_LEN_ERR_EID        4   /**< 명령 길이 오류                    */
#define IG4U_PIPE_ERR_EID           5   /**< SB Pipe 오류                      */
#define IG4U_CR_PIPE_ERR_EID        6   /**< Pipe 생성 오류                    */
#define IG4U_SUB_CMD_ERR_EID        7   /**< CMD MID 구독 오류                 */
#define IG4U_SUB_WAKEUP_ERR_EID     8   /**< WAKEUP MID 구독 오류              */
#define IG4U_TBL_ERR_EID            9   /**< 테이블 등록 오류                  */
#define IG4U_SEM_INIT_ERR_EID       10  /**< 세마포어 초기화 오류              */
#define IG4U_MUT_INIT_ERR_EID       11  /**< 뮤텍스 초기화 오류               */
#define IG4U_CHILD_CREATE_ERR_EID   12  /**< 자식 태스크 생성 오류             */

/* ------------------------------------------------------------------
 * 20~29: RS-422 / UART 통신 오류
 * ------------------------------------------------------------------ */
#define IG4U_UART_SEND_ERR_EID      20  /**< UART 송신 오류                    */
#define IG4U_UART_RECV_ERR_EID      21  /**< UART 수신 오류                    */
#define IG4U_UART_CRC_ERR_EID       22  /**< CRC-16 검증 실패                  */
#define IG4U_UART_TIMEOUT_ERR_EID   23  /**< 수신 타임아웃                     */
#define IG4U_UART_SYNC_ERR_EID      24  /**< 헤더 동기화 실패 (0xAA55 미발견)  */
#define IG4U_UART_NACK_ERR_EID      25  /**< 탑재체로부터 NACK 수신            */

/* ------------------------------------------------------------------
 * 30~39: 상태 머신 전이 이벤트
 * IG4U-SW-ICD-001 Section 5.1
 * ------------------------------------------------------------------ */
#define IG4U_STATE_BOOT_INF_EID     30  /**< BOOT 상태 진입                    */
#define IG4U_STATE_STANDBY_INF_EID  31  /**< STANDBY 상태 전이                 */
#define IG4U_STATE_ARMED_INF_EID    32  /**< ARMED 상태 전이                   */
#define IG4U_STATE_FIRING_INF_EID   33  /**< FIRING 상태 진입                  */
#define IG4U_STATE_SAFE_ERR_EID     34  /**< SAFE 상태 전이 (보호 상태 진입)   */
#define IG4U_STATE_FAULT_ERR_EID    35  /**< FAULT 상태 전이 (치명적 오류)     */
#define IG4U_ARMED_TIMEOUT_ERR_EID  36  /**< ARMED 타임아웃 → STANDBY 자동복귀 */

/* ------------------------------------------------------------------
 * 40~49: 추진계 구동 이벤트
 * ------------------------------------------------------------------ */
#define IG4U_MAIN_FIRE_INF_EID      40  /**< 주추력기 분사 시작                */
#define IG4U_MAIN_FIRE_DONE_INF_EID 41  /**< 주추력기 분사 완료                */
#define IG4U_MAIN_ABORT_INF_EID     42  /**< 주추력기 분사 중단(Abort)         */
#define IG4U_CG_FIRE_INF_EID        43  /**< 냉가스 추력기 분사 시작           */
#define IG4U_CG_FIRE_DONE_INF_EID   44  /**< 냉가스 추력기 분사 완료           */
#define IG4U_CG_ABORT_INF_EID       45  /**< 냉가스 추력기 분사 중단(Abort)    */
#define IG4U_INTERLOCK_ERR_EID      46  /**< 인터록 조건 미충족 (ARM 거부)     */

/* ------------------------------------------------------------------
 * 50~59: Fault 관리 이벤트
 * ------------------------------------------------------------------ */
#define IG4U_FAULT_DETECTED_ERR_EID 50  /**< 새로운 Fault 감지                 */
#define IG4U_FAULT_CLEARED_INF_EID  51  /**< Fault 해제 성공                   */
#define IG4U_FAULT_CLEAR_FAIL_EID   52  /**< Fault 해제 실패 (조건 미충족)     */
#define IG4U_PRESSURE_HIGH_ERR_EID  53  /**< 탱크 압력 초과                    */
#define IG4U_TEMP_HIGH_ERR_EID      54  /**< 온도 초과                         */

#endif /* IG4U_EVENTIDS_H */

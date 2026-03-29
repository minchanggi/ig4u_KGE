#ifndef IG4U_TOPICIDS_H
#define IG4U_TOPICIDS_H

/**
 * @file ig4u_topicids.h
 * @brief iG4U 추력 유닛 cFS Message Topic IDs
 */

/* Command Topic IDs */
#define CFE_MISSION_IG4U_CMD_TOPICID          0xB0
#define CFE_MISSION_IG4U_WAKEUP_TOPICID       0xB1

/* Telemetry Topic IDs */
#define CFE_MISSION_IG4U_HK_TLM_TOPICID       0xB0  /**< HK 데이터 텔레메트리  */
#define CFE_MISSION_IG4U_STATUS_TLM_TOPICID   0xB1  /**< 상태 텔레메트리       */
#define CFE_MISSION_IG4U_FIRE_RESULT_TOPICID  0xB2  /**< 분사 결과 텔레메트리  */
#define CFE_MISSION_IG4U_FAULT_LOG_TOPICID    0xB3  /**< Fault 로그 텔레메트리 */

#endif /* IG4U_TOPICIDS_H */

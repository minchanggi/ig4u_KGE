#ifndef IG4U_INTERFACE_CFG_H
#define IG4U_INTERFACE_CFG_H

/**
 * @file ig4u_interface_cfg.h
 * @brief iG4U 추력 유닛 물리 인터페이스 설정
 *
 * 참조: IG4U-SW-ICD-001 Section 3 (물리 및 데이터링크 계층)
 *
 * 물리 계층: RS-422 차동 신호
 * 통신 방식: UART over RS-422
 */

#include "common_types.h"
#include "cfe.h"

/* ===================================================================
 * RS-422 / UART Interface Configuration
 * ICD Section 3.2: UART 프로토콜 설정
 * =================================================================== */
#define IG4U_UART_HANDLE_INDEXER    CFE_SRL_UART2_HANDLE_INDEXER  /**< [TBD] UART 핸들 인덱서 */
#define IG4U_UART_BAUD_RATE         921600U   /**< 921,600 bps (TBD per ICD)          */
#define IG4U_UART_DATA_BITS         8         /**< 데이터 비트: 8                     */
#define IG4U_UART_PARITY            0         /**< 패리티: No (0=None)                */
#define IG4U_UART_STOP_BITS         1         /**< 스톱 비트: 1                       */
#define IG4U_UART_SEND_ORDER_LSB    1         /**< LSB First                          */
#define IG4U_UART_TIMEOUT_MS        500U      /**< 수신 타임아웃 (ms)                 */

/* ===================================================================
 * Packet Frame Definition
 * ICD Section 4.1: 메시지 형식
 * =================================================================== */
#define IG4U_PACKET_HEADER_MAGIC    0xAA55U   /**< 패킷 헤더 동기 워드               */
#define IG4U_PACKET_VERSION         0x10U     /**< 버전: Major=1, Minor=0            */

/* Packet overhead (bytes): Header(2)+Version(1)+MsgType(1)+MsgID(1)+Length(2)+CRC(2) */
#define IG4U_PACKET_OVERHEAD_BYTES  9U
#define IG4U_PACKET_MAX_PAYLOAD     256U      /**< 최대 페이로드 크기 (bytes)         */
#define IG4U_PACKET_MAX_TOTAL       (IG4U_PACKET_OVERHEAD_BYTES + IG4U_PACKET_MAX_PAYLOAD)

/* MsgType values */
#define IG4U_MSGTYPE_COMMAND        0U        /**< Telecommand (TC)                   */
#define IG4U_MSGTYPE_RESPONSE       1U        /**< Telemetry Response (TM)            */
#define IG4U_MSGTYPE_EVENT          2U        /**< Event 메시지                       */

/* ===================================================================
 * Wire-level Message IDs (iG4U 고유, ICD Table 2)
 * cFS Function Code와 구분됨에 주의
 * =================================================================== */
#define IG4U_WIRE_MSGID_PING                1U
#define IG4U_WIRE_MSGID_RESET_MODULE        2U
#define IG4U_WIRE_MSGID_SET_MODE            3U
#define IG4U_WIRE_MSGID_ARM_PROPULSION      4U
#define IG4U_WIRE_MSGID_DISARM_PROPULSION   5U
#define IG4U_WIRE_MSGID_REQUEST_HK          10U
#define IG4U_WIRE_MSGID_REPLY_HK            10U
#define IG4U_WIRE_MSGID_REQUEST_STATUS      11U
#define IG4U_WIRE_MSGID_REPLY_STATUS        11U
#define IG4U_WIRE_MSGID_MAIN_FIRE           20U
#define IG4U_WIRE_MSGID_MAIN_ABORT          21U
#define IG4U_WIRE_MSGID_MAIN_RESULT         20U
#define IG4U_WIRE_MSGID_CG_PULSE            30U
#define IG4U_WIRE_MSGID_CG_BURST            31U
#define IG4U_WIRE_MSGID_CG_ABORT            32U
#define IG4U_WIRE_MSGID_CG_RESULT           30U  /**< Response MsgID = triggering TC MsgID */
#define IG4U_WIRE_MSGID_REQUEST_FAULT_LOG   40U
#define IG4U_WIRE_MSGID_REPLY_FAULT_LOG     40U
#define IG4U_WIRE_MSGID_CLEAR_FAULT         41U

/* ===================================================================
 * CRC-16 Configuration (CCSDS/CCITT)
 * ICD Section 4.1: CRC-16 예시
 * Polynomial: 0x1021, Initial Value: 0xFFFF
 * =================================================================== */
#define IG4U_CRC16_POLY             0x1021U
#define IG4U_CRC16_INIT             0xFFFFU

/* ===================================================================
 * Power Interface (TBD: 위성 본체 전원 채널)
 * =================================================================== */
#define IG4U_POWER_CHANNEL          22        /**< [TBD] 전원 채널 번호              */
#define IG4U_POWER_CONVERTER        0

/* ===================================================================
 * Armed State Timeout
 * ARMED 상태에서 일정 시간 내 Fire 명령 없을 시 자동 STANDBY 복귀
 * =================================================================== */
#define IG4U_ARMED_TIMEOUT_SEC      30U       /**< [TBD] Armed 타임아웃 (초)         */

/* ===================================================================
 * HK Data Channel Counts
 * ICD Table 2: Reply HK Data - 압력 2채널 + 온도 4채널
 * =================================================================== */
#define IG4U_HK_PRESSURE_CH_COUNT   2U
#define IG4U_HK_TEMP_CH_COUNT       4U

/* ===================================================================
 * Fault Log Configuration
 * =================================================================== */
#define IG4U_FAULT_LOG_MAX_ENTRIES  32U

#endif /* IG4U_INTERFACE_CFG_H */

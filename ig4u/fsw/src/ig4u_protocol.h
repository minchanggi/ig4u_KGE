#ifndef IG4U_PROTOCOL_H
#define IG4U_PROTOCOL_H

/**
 * @file ig4u_protocol.h
 * @brief iG4U RS-422 패킷 프로토콜 레이어
 *
 * 참조: IG4U-SW-ICD-001 Section 3~4
 *
 * 패킷 구조:
 *   Byte[0-1]  : Header (0xAA, 0x55)
 *   Byte[2]    : Version (Major[7:4] | Minor[3:0])
 *   Byte[3]    : MsgType (0=TC, 1=TM Response, 2=Event)
 *   Byte[4]    : MsgID
 *   Byte[5-6]  : Length (Payload 길이, Little-Endian)
 *   Byte[7..N] : Payload
 *   Byte[N+1-N+2] : CRC-16 (CCSDS, Polynomial=0x1021, Init=0xFFFF)
 *
 * CRC 계산 범위: Header ~ Payload 마지막 바이트 (CRC 필드 제외)
 */

#include "common_types.h"
#include "ig4u_interface_cfg.h"

/* ===================================================================
 * 반환 코드
 * =================================================================== */
#define IG4U_PROTO_OK               0
#define IG4U_PROTO_ERR_NULL_PTR    -1
#define IG4U_PROTO_ERR_BUF_SIZE    -2   /**< 버퍼 크기 부족              */
#define IG4U_PROTO_ERR_CRC_FAIL    -3   /**< CRC 검증 실패               */
#define IG4U_PROTO_ERR_BAD_HEADER  -4   /**< 동기 헤더 불일치            */
#define IG4U_PROTO_ERR_BAD_LENGTH  -5   /**< Length 필드 이상            */

/* ===================================================================
 * 패킷 원시(raw) 구조체
 * 전송/수신 버퍼를 직접 매핑하는 용도로 사용
 * =================================================================== */
typedef struct {
    uint8_t  Header[2];     /**< 0xAA, 0x55                             */
    uint8_t  Version;       /**< Major[7:4] | Minor[3:0]                */
    uint8_t  MsgType;       /**< 0=TC, 1=Response, 2=Event              */
    uint8_t  MsgID;         /**< Message ID                             */
    uint16_t Length;        /**< Payload 바이트 수 (Little-Endian)      */
    uint8_t  Payload[IG4U_PACKET_MAX_PAYLOAD]; /**< 가변 페이로드       */
    /* CRC16은 Payload[Length] ~ [Length+1]에 위치 (가변) */
} __attribute__((packed)) IG4U_RawPacket_t;

/* ===================================================================
 * API
 * =================================================================== */

/**
 * @brief CRC-16 계산 (CCSDS/CCITT, Poly=0x1021, Init=0xFFFF)
 * @param data   계산 대상 데이터 포인터
 * @param length 데이터 길이 (bytes)
 * @return 계산된 CRC-16 값
 */
uint16_t IG4U_Proto_CalcCRC16(const uint8_t *data, uint32_t length);

/**
 * @brief TC(명령) 패킷 인코딩
 *
 * payload_buf를 포함하는 완전한 RS-422 송신 패킷을 out_buf에 생성합니다.
 * CRC는 자동 계산되어 패킷 끝에 추가됩니다.
 *
 * @param out_buf       출력 버퍼 (최소 IG4U_PACKET_OVERHEAD_BYTES + payload_len)
 * @param out_buf_size  출력 버퍼 크기
 * @param msg_id        Wire-level MsgID (IG4U_WIRE_MSGID_xxx)
 * @param payload_buf   페이로드 데이터 (NULL 가능, payload_len=0 시)
 * @param payload_len   페이로드 길이 (bytes)
 * @return 인코딩된 패킷 전체 길이 (bytes), 오류 시 음수
 */
int32_t IG4U_Proto_EncodeTC(uint8_t        *out_buf,
                             uint32_t        out_buf_size,
                             uint8_t         msg_id,
                             const uint8_t  *payload_buf,
                             uint16_t        payload_len);

/**
 * @brief TM(응답) 패킷 디코딩 및 검증
 *
 * 수신된 raw 바이트 스트림을 검증하고 페이로드를 추출합니다.
 * 검증 항목: 헤더 동기, Length 범위, CRC-16
 *
 * @param in_buf        수신 버퍼
 * @param in_buf_len    수신 버퍼 내 유효 데이터 길이
 * @param out_msg_type  [출력] MsgType (0/1/2)
 * @param out_msg_id    [출력] MsgID
 * @param out_payload   [출력] 페이로드 포인터 (in_buf 내부를 가리킴)
 * @param out_payload_len [출력] 페이로드 길이
 * @return IG4U_PROTO_OK 또는 오류 코드
 */
int32_t IG4U_Proto_DecodeTM(const uint8_t *in_buf,
                             uint32_t       in_buf_len,
                             uint8_t       *out_msg_type,
                             uint8_t       *out_msg_id,
                             const uint8_t **out_payload,
                             uint16_t      *out_payload_len);

/**
 * @brief 수신 버퍼에서 패킷 헤더(0xAA55) 동기화 위치 탐색
 *
 * 통신 재연결 또는 쓰레기 데이터 수신 후 복구에 사용합니다.
 * ICD Section 6.1 (10)번 시험 절차 대응.
 *
 * @param buf     검색할 버퍼
 * @param buf_len 버퍼 길이
 * @return 헤더 시작 인덱스, 미발견 시 -1
 */
int32_t IG4U_Proto_FindSyncHeader(const uint8_t *buf, uint32_t buf_len);

#endif /* IG4U_PROTOCOL_H */

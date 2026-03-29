/**
 * @file ig4u_protocol.c
 * @brief iG4U RS-422 패킷 프로토콜 레이어 구현
 *
 * 참조: IG4U-SW-ICD-001 Section 3~4
 */

#include "ig4u_protocol.h"
#include "ig4u_internal_cfg.h"
#include <string.h>  /* memcpy, memset */

/* ===================================================================
 * 내부 상수
 * =================================================================== */
#define HEADER_BYTE_0   0xAAU
#define HEADER_BYTE_1   0x55U

/* 패킷 내 각 필드의 바이트 오프셋 */
#define OFFSET_HEADER   0U
#define OFFSET_VERSION  2U
#define OFFSET_MSGTYPE  3U
#define OFFSET_MSGID    4U
#define OFFSET_LENGTH   5U  /* 2바이트, Little-Endian */
#define OFFSET_PAYLOAD  7U

/* ===================================================================
 * CRC-16 계산 (CCSDS/CCITT)
 * Polynomial: 0x1021  (x^16 + x^12 + x^5 + 1)
 * Initial:    0xFFFF
 * ICD Section 4.1에 제시된 알고리즘과 동일
 * =================================================================== */
uint16_t IG4U_Proto_CalcCRC16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = IG4U_CRC16_INIT;  /* 0xFFFF */
    uint32_t i;
    uint8_t  j;

    if (data == NULL)
    {
        return 0U;
    }

    for (i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);
        for (j = 0U; j < 8U; j++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1U) ^ IG4U_CRC16_POLY);
            }
            else
            {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }
    return crc;
}

/* ===================================================================
 * TC 패킷 인코딩
 * =================================================================== */
int32_t IG4U_Proto_EncodeTC(uint8_t        *out_buf,
                             uint32_t        out_buf_size,
                             uint8_t         msg_id,
                             const uint8_t  *payload_buf,
                             uint16_t        payload_len)
{
    uint32_t total_len;
    uint16_t crc;

    /* 인자 검사 */
    if (out_buf == NULL)
    {
        return IG4U_PROTO_ERR_NULL_PTR;
    }

    /* 페이로드가 있는데 포인터가 NULL인 경우 */
    if ((payload_len > 0U) && (payload_buf == NULL))
    {
        return IG4U_PROTO_ERR_NULL_PTR;
    }

    /* 최대 페이로드 크기 검사 */
    if (payload_len > IG4U_PACKET_MAX_PAYLOAD)
    {
        return IG4U_PROTO_ERR_BAD_LENGTH;
    }

    /* 전체 패킷 크기: Header(2) + Version(1) + MsgType(1) + MsgID(1)
     *                 + Length(2) + Payload(N) + CRC16(2)             */
    total_len = (uint32_t)IG4U_PACKET_OVERHEAD_BYTES + (uint32_t)payload_len;

    if (out_buf_size < total_len)
    {
        return IG4U_PROTO_ERR_BUF_SIZE;
    }

    /* --- 패킷 조립 --- */
    memset(out_buf, 0, total_len);

    /* Header 동기 워드 */
    out_buf[OFFSET_HEADER + 0U] = HEADER_BYTE_0;  /* 0xAA */
    out_buf[OFFSET_HEADER + 1U] = HEADER_BYTE_1;  /* 0x55 */

    /* Version */
    out_buf[OFFSET_VERSION] = IG4U_PACKET_VERSION;

    /* MsgType: TC = 0 */
    out_buf[OFFSET_MSGTYPE] = IG4U_MSGTYPE_COMMAND;

    /* MsgID */
    out_buf[OFFSET_MSGID] = msg_id;

    /* Length (Little-Endian) */
    out_buf[OFFSET_LENGTH + 0U] = (uint8_t)(payload_len & 0xFFU);
    out_buf[OFFSET_LENGTH + 1U] = (uint8_t)((payload_len >> 8U) & 0xFFU);

    /* Payload 복사 */
    if ((payload_len > 0U) && (payload_buf != NULL))
    {
        memcpy(&out_buf[OFFSET_PAYLOAD], payload_buf, payload_len);
    }

    /* CRC-16 계산: Header ~ Payload 끝까지 */
    crc = IG4U_Proto_CalcCRC16(out_buf, (uint32_t)(OFFSET_PAYLOAD + payload_len));

    /* CRC 삽입 (Little-Endian) */
    out_buf[OFFSET_PAYLOAD + payload_len + 0U] = (uint8_t)(crc & 0xFFU);
    out_buf[OFFSET_PAYLOAD + payload_len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);

    IG4U_PRINTF("[IG4U_PROTO] EncodeTC MsgID=0x%02X PayloadLen=%u TotalLen=%u CRC=0x%04X\n",
                msg_id, payload_len, (unsigned)total_len, crc);

    return (int32_t)total_len;
}

/* ===================================================================
 * TM 패킷 디코딩 및 검증
 * =================================================================== */
int32_t IG4U_Proto_DecodeTM(const uint8_t *in_buf,
                             uint32_t       in_buf_len,
                             uint8_t       *out_msg_type,
                             uint8_t       *out_msg_id,
                             const uint8_t **out_payload,
                             uint16_t      *out_payload_len)
{
    uint16_t length_field;
    uint32_t expected_total;
    uint16_t crc_received;
    uint16_t crc_calculated;

    /* 인자 검사 */
    if ((in_buf == NULL) || (out_msg_type == NULL) || (out_msg_id == NULL) ||
        (out_payload == NULL) || (out_payload_len == NULL))
    {
        return IG4U_PROTO_ERR_NULL_PTR;
    }

    /* 최소 패킷 크기: overhead 전체(페이로드 0바이트 기준) */
    if (in_buf_len < IG4U_PACKET_OVERHEAD_BYTES)
    {
        return IG4U_PROTO_ERR_BAD_LENGTH;
    }

    /* 헤더 동기 검사 */
    if ((in_buf[OFFSET_HEADER + 0U] != HEADER_BYTE_0) ||
        (in_buf[OFFSET_HEADER + 1U] != HEADER_BYTE_1))
    {
        IG4U_PRINTF("[IG4U_PROTO] BAD HEADER: 0x%02X 0x%02X\n",
                    in_buf[0], in_buf[1]);
        return IG4U_PROTO_ERR_BAD_HEADER;
    }

    /* Length 필드 읽기 (Little-Endian) */
    length_field = (uint16_t)in_buf[OFFSET_LENGTH + 0U] |
                   (uint16_t)((uint16_t)in_buf[OFFSET_LENGTH + 1U] << 8U);

    /* 페이로드 크기 범위 검사 */
    if (length_field > IG4U_PACKET_MAX_PAYLOAD)
    {
        return IG4U_PROTO_ERR_BAD_LENGTH;
    }

    /* 전체 예상 패킷 크기 */
    expected_total = (uint32_t)IG4U_PACKET_OVERHEAD_BYTES + (uint32_t)length_field;

    if (in_buf_len < expected_total)
    {
        return IG4U_PROTO_ERR_BAD_LENGTH;
    }

    /* CRC 검증 */
    crc_received = (uint16_t)in_buf[OFFSET_PAYLOAD + length_field + 0U] |
                   (uint16_t)((uint16_t)in_buf[OFFSET_PAYLOAD + length_field + 1U] << 8U);

    crc_calculated = IG4U_Proto_CalcCRC16(in_buf,
                                           (uint32_t)(OFFSET_PAYLOAD + length_field));

    if (crc_received != crc_calculated)
    {
        IG4U_PRINTF("[IG4U_PROTO] CRC FAIL: received=0x%04X calculated=0x%04X\n",
                    crc_received, crc_calculated);
        return IG4U_PROTO_ERR_CRC_FAIL;
    }

    /* 출력 */
    *out_msg_type     = in_buf[OFFSET_MSGTYPE];
    *out_msg_id       = in_buf[OFFSET_MSGID];
    *out_payload      = &in_buf[OFFSET_PAYLOAD];
    *out_payload_len  = length_field;

    IG4U_PRINTF("[IG4U_PROTO] DecodeTM OK MsgType=%u MsgID=0x%02X PayloadLen=%u\n",
                *out_msg_type, *out_msg_id, *out_payload_len);

    return IG4U_PROTO_OK;
}

/* ===================================================================
 * 수신 버퍼에서 동기 헤더(0xAA55) 탐색
 * =================================================================== */
int32_t IG4U_Proto_FindSyncHeader(const uint8_t *buf, uint32_t buf_len)
{
    uint32_t i;

    if (buf == NULL)
    {
        return -1;
    }

    /* 마지막 바이트는 단독으로 헤더 완성 불가이므로 len-1까지 탐색 */
    if (buf_len < 2U)
    {
        return -1;
    }

    for (i = 0U; i < (buf_len - 1U); i++)
    {
        if ((buf[i] == HEADER_BYTE_0) && (buf[i + 1U] == HEADER_BYTE_1))
        {
            return (int32_t)i;
        }
    }

    return -1;
}

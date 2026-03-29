/**
 * @file ig4u_commands.c
 * @brief iG4U TC 명령 빌더 및 TM 응답 파서 구현
 *
 * 참조: IG4U-SW-ICD-001 Section 4.2, 6.1
 *
 * 엔디안 처리 방침:
 *   TC 송신: IG4U_WR_LE16 / IG4U_WR_LE32 로 바이트 직렬화
 *   TM 수신: IG4U_RD_LE16 / IG4U_RD_LE32 로 바이트 역직렬화
 *   → struct memcpy 사용 금지 (OBC vs iG4U MCU 엔디안 불일치 방지)
 *   → ig4u_endian.h 의 IG4U_WIRE_BIG_ENDIAN 정의로 와이어 엔디안 전환 가능
 */

#include "ig4u_commands.h"
#include "ig4u_protocol.h"
#include "ig4u_interface_cfg.h"
#include "ig4u_internal_cfg.h"
#include "ig4u_endian.h"
#include "ig4u_hal.h"
#include <string.h>

/* ===================================================================
 * 내부 송수신 버퍼
 * =================================================================== */
static uint8_t s_tx_buf[IG4U_PACKET_MAX_TOTAL];
static uint8_t s_rx_buf[IG4U_PACKET_MAX_TOTAL];

/* ===================================================================
 * 내부 헬퍼: TC 전송
 * =================================================================== */
static int32_t SendTC(uint8_t msg_id, const uint8_t *payload, uint16_t payload_len)
{
    int32_t encoded_len;

    encoded_len = IG4U_Proto_EncodeTC(s_tx_buf, sizeof(s_tx_buf),
                                       msg_id, payload, payload_len);
    if (encoded_len < 0)
    {
        IG4U_PRINTF("[IG4U_CMD] EncodeTC failed: %d\n", (int)encoded_len);
        return IG4U_CMD_ERR_ENCODE;
    }

    if (IG4U_HAL_Send(s_tx_buf, (uint32_t)encoded_len) != IG4U_HAL_OK)
    {
        IG4U_PRINTF("[IG4U_CMD] HAL_Send failed\n");
        return IG4U_CMD_ERR_SEND;
    }

    return IG4U_CMD_OK;
}

/* ===================================================================
 * 내부 헬퍼: TM 수신 후 raw payload 포인터 반환
 * =================================================================== */
static int32_t RecvTMRaw(uint8_t         expected_msg_id,
                          const uint8_t **out_payload,
                          uint16_t       *out_payload_len)
{
    int32_t recv_len;
    int32_t ret;
    uint8_t msg_type;
    uint8_t msg_id;

    memset(s_rx_buf, 0, sizeof(s_rx_buf));

    recv_len = IG4U_HAL_Recv(s_rx_buf, sizeof(s_rx_buf), IG4U_UART_TIMEOUT_MS);
    if (recv_len <= 0)
    {
        IG4U_PRINTF("[IG4U_CMD] Recv timeout. MsgID=0x%02X\n", expected_msg_id);
        return IG4U_CMD_ERR_TIMEOUT;
    }

    ret = IG4U_Proto_DecodeTM(s_rx_buf, (uint32_t)recv_len,
                               &msg_type, &msg_id,
                               out_payload, out_payload_len);
    if (ret != IG4U_PROTO_OK)
    {
        IG4U_PRINTF("[IG4U_CMD] DecodeTM failed: %d\n", (int)ret);
        return IG4U_CMD_ERR_PARSE;
    }

    if (msg_id != expected_msg_id)
    {
        IG4U_PRINTF("[IG4U_CMD] MsgID mismatch: exp=0x%02X got=0x%02X\n",
                    expected_msg_id, msg_id);
        return IG4U_CMD_ERR_PARSE;
    }

    return IG4U_CMD_OK;
}

/* ===================================================================
 * Default Response 수신 대기 및 파싱
 *
 * 페이로드 레이아웃 (모두 1바이트 → 엔디안 무관):
 *   [0] Result    (uint8)
 *   [1] EchoMsgID (uint8)
 *   [2] ErrorCode (uint8)
 *   [3] Spare     (uint8)
 * =================================================================== */
int32_t IG4U_Cmd_WaitDefaultResponse(uint8_t                         expected_msg_id,
                                       IG4U_DefaultResponse_Payload_t *out_resp)
{
    int32_t        ret;
    const uint8_t *p;
    uint16_t       plen;

    ret = RecvTMRaw(expected_msg_id, &p, &plen);
    if (ret != IG4U_CMD_OK) { return ret; }

    if (out_resp != NULL)
    {
        if (plen < 3U) { return IG4U_CMD_ERR_PARSE; }

        out_resp->Result    = p[0];
        out_resp->EchoMsgID = p[1];
        out_resp->ErrorCode = p[2];
        out_resp->Spare     = 0U;

        if (out_resp->Result != IG4U_RESULT_ACK)
        {
            IG4U_PRINTF("[IG4U_CMD] NACK: MsgID=0x%02X ErrCode=0x%02X\n",
                        expected_msg_id, out_resp->ErrorCode);
            return IG4U_CMD_ERR_NACK;
        }
    }

    return IG4U_CMD_OK;
}

/* ===================================================================
 * Group 1: 모듈 제어
 * =================================================================== */

int32_t IG4U_Cmd_Ping(void)
{
    int32_t ret;
    IG4U_PRINTF("[IG4U_CMD] >> PING\n");
    ret = SendTC(IG4U_WIRE_MSGID_PING, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_PING, NULL);
}

int32_t IG4U_Cmd_ResetModule(void)
{
    int32_t ret;
    IG4U_PRINTF("[IG4U_CMD] >> RESET MODULE\n");
    ret = SendTC(IG4U_WIRE_MSGID_RESET_MODULE, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_RESET_MODULE, NULL);
}

int32_t IG4U_Cmd_SetMode(uint8_t mode)
{
    int32_t ret;
    uint8_t buf[4];

    IG4U_PRINTF("[IG4U_CMD] >> SET MODE: %u\n", mode);

    /* 페이로드: Mode(1) + Spare(3) - 모두 1바이트, 엔디안 무관 */
    buf[0] = mode;
    buf[1] = 0U;
    buf[2] = 0U;
    buf[3] = 0U;

    ret = SendTC(IG4U_WIRE_MSGID_SET_MODE, buf, (uint16_t)sizeof(buf));
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_SET_MODE, NULL);
}

int32_t IG4U_Cmd_ArmPropulsion(void)
{
    int32_t ret;
    IG4U_PRINTF("[IG4U_CMD] >> ARM PROPULSION\n");
    ret = SendTC(IG4U_WIRE_MSGID_ARM_PROPULSION, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_ARM_PROPULSION, NULL);
}

int32_t IG4U_Cmd_DisarmPropulsion(void)
{
    int32_t ret;
    IG4U_PRINTF("[IG4U_CMD] >> DISARM PROPULSION\n");
    ret = SendTC(IG4U_WIRE_MSGID_DISARM_PROPULSION, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_DISARM_PROPULSION, NULL);
}

/* ===================================================================
 * Group 2: HK / 상태
 * =================================================================== */

/**
 * Reply HK Data TM 페이로드 레이아웃:
 *   [0-1]   Pressure_kPa_x10[0]  (uint16 LE)
 *   [2-3]   Pressure_kPa_x10[1]  (uint16 LE)
 *   [4-5]   Temperature_x10[0]   (int16  LE)
 *   [6-7]   Temperature_x10[1]   (int16  LE)
 *   [8-9]   Temperature_x10[2]   (int16  LE)
 *   [10-11] Temperature_x10[3]   (int16  LE)
 *   [12]    Mode                 (uint8)
 *   [13]    FaultFlags_Lo        (uint8)
 *   [14]    FaultFlags_Hi        (uint8)
 *   [15]    Spare                (uint8)
 */
int32_t IG4U_Cmd_RequestHKData(IG4U_ReplyHKData_Payload_t *out_hk)
{
    int32_t        ret;
    const uint8_t *p;
    uint16_t       plen;

    if (out_hk == NULL) { return IG4U_CMD_ERR_PARSE; }

    IG4U_PRINTF("[IG4U_CMD] >> REQUEST HK DATA\n");
    ret = SendTC(IG4U_WIRE_MSGID_REQUEST_HK, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }

    ret = RecvTMRaw(IG4U_WIRE_MSGID_REPLY_HK, &p, &plen);
    if (ret != IG4U_CMD_OK) { return ret; }

    if (plen < 16U) { return IG4U_CMD_ERR_PARSE; }

    /* 명시적 바이트 역직렬화 */
    out_hk->Pressure_kPa_x10[0] = IG4U_RD_LE16(&p[0]);
    out_hk->Pressure_kPa_x10[1] = IG4U_RD_LE16(&p[2]);
    out_hk->Temperature_x10[0]  = IG4U_RD_LE16S(&p[4]);
    out_hk->Temperature_x10[1]  = IG4U_RD_LE16S(&p[6]);
    out_hk->Temperature_x10[2]  = IG4U_RD_LE16S(&p[8]);
    out_hk->Temperature_x10[3]  = IG4U_RD_LE16S(&p[10]);
    out_hk->Mode                = p[12];
    out_hk->FaultFlags_Lo       = p[13];
    out_hk->FaultFlags_Hi       = p[14];
    out_hk->Spare               = 0U;

    return IG4U_CMD_OK;
}

/**
 * Reply Status TM 페이로드 레이아웃:
 *   [0]   Mode               (uint8)
 *   [1]   InterlockStatus    (uint8)
 *   [2-3] FaultFlags         (uint16 LE)
 *   [4]   ArmTimeout_Remain  (uint8)
 *   [5-7] Spare              (uint8 x3)
 */
int32_t IG4U_Cmd_RequestStatus(IG4U_ReplyStatus_Payload_t *out_status)
{
    int32_t        ret;
    const uint8_t *p;
    uint16_t       plen;

    if (out_status == NULL) { return IG4U_CMD_ERR_PARSE; }

    IG4U_PRINTF("[IG4U_CMD] >> REQUEST STATUS\n");
    ret = SendTC(IG4U_WIRE_MSGID_REQUEST_STATUS, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }

    ret = RecvTMRaw(IG4U_WIRE_MSGID_REPLY_STATUS, &p, &plen);
    if (ret != IG4U_CMD_OK) { return ret; }

    if (plen < 5U) { return IG4U_CMD_ERR_PARSE; }

    out_status->Mode              = p[0];
    out_status->InterlockStatus   = p[1];
    out_status->FaultFlags        = IG4U_RD_LE16(&p[2]);
    out_status->ArmTimeout_Remain = p[4];

    return IG4U_CMD_OK;
}

/* ===================================================================
 * Group 3: 주추력기
 * =================================================================== */

/**
 * Main Thruster Fire TC 페이로드:
 *   [0-3] Duration_ms (uint32 LE)
 *
 * Main Thruster Result TM 페이로드:
 *   [0]   Result             (uint8)
 *   [1]   Spare              (uint8)
 *   [2-3] Actual_Duration_ms (uint16 LE)
 *   [4-5] FaultFlags         (uint16 LE)
 *   [6-7] Spare2             (uint8 x2)
 */
int32_t IG4U_Cmd_MainThrusterFire(uint32_t                          duration_ms,
                                   IG4U_MainThrusterResult_Payload_t *out_result)
{
    int32_t        ret;
    uint8_t        buf[4];
    const uint8_t *p;
    uint16_t       plen;

    IG4U_PRINTF("[IG4U_CMD] >> MAIN THRUSTER FIRE: duration=%u ms\n",
                (unsigned)duration_ms);

    IG4U_WR_LE32(buf, duration_ms);

    ret = SendTC(IG4U_WIRE_MSGID_MAIN_FIRE, buf, (uint16_t)sizeof(buf));
    if (ret != IG4U_CMD_OK) { return ret; }

    ret = IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_MAIN_FIRE, NULL);
    if (ret != IG4U_CMD_OK) { return ret; }

    if ((duration_ms > 0U) && (out_result != NULL))
    {
        ret = RecvTMRaw(IG4U_WIRE_MSGID_MAIN_RESULT, &p, &plen);
        if (ret != IG4U_CMD_OK) { return ret; }
        if (plen < 6U) { return IG4U_CMD_ERR_PARSE; }

        out_result->Result             = p[0];
        out_result->Spare              = 0U;
        out_result->Actual_Duration_ms = IG4U_RD_LE16(&p[2]);
        out_result->FaultFlags         = IG4U_RD_LE16(&p[4]);

        IG4U_PRINTF("[IG4U_CMD] Main Fire Result: result=%u actual=%u ms\n",
                    out_result->Result, out_result->Actual_Duration_ms);
    }

    return IG4U_CMD_OK;
}

int32_t IG4U_Cmd_MainThrusterAbort(void)
{
    IG4U_PRINTF("[IG4U_CMD] >> MAIN THRUSTER ABORT\n");
    return SendTC(IG4U_WIRE_MSGID_MAIN_ABORT, NULL, 0U);
}

/* ===================================================================
 * Group 4: 냉가스 추력기
 * =================================================================== */

/**
 * CG Thruster Pulse TC 페이로드:
 *   [0]   ThrusterID  (uint8)
 *   [1]   Spare       (uint8)
 *   [2-3] Duration_ms (uint16 LE)
 *
 * CG Thruster Result TM 페이로드:
 *   [0]   TriggerMsgID       (uint8)
 *   [1]   ThrusterID         (uint8)
 *   [2]   Result             (uint8)
 *   [3]   Spare              (uint8)
 *   [4-5] ExecutedPulseCount (uint16 LE)
 *   [6-7] FaultFlags         (uint16 LE)
 */
int32_t IG4U_Cmd_CGThrusterPulse(uint8_t                         thruster_id,
                                   uint16_t                        duration_ms,
                                   IG4U_CGThrusterResult_Payload_t *out_result)
{
    int32_t        ret;
    uint8_t        buf[4];
    const uint8_t *p;
    uint16_t       plen;

    IG4U_PRINTF("[IG4U_CMD] >> CG PULSE: ch=%u dur=%u ms\n",
                thruster_id, duration_ms);

    buf[0] = thruster_id;
    buf[1] = 0U;
    IG4U_WR_LE16(&buf[2], duration_ms);

    ret = SendTC(IG4U_WIRE_MSGID_CG_PULSE, buf, (uint16_t)sizeof(buf));
    if (ret != IG4U_CMD_OK) { return ret; }

    if (out_result != NULL)
    {
        ret = RecvTMRaw(IG4U_WIRE_MSGID_CG_RESULT, &p, &plen);
        if (ret != IG4U_CMD_OK) { return ret; }
        if (plen < 8U) { return IG4U_CMD_ERR_PARSE; }

        out_result->TriggerMsgID       = p[0];
        out_result->ThrusterID         = p[1];
        out_result->Result             = p[2];
        out_result->Spare              = 0U;
        out_result->ExecutedPulseCount = IG4U_RD_LE16(&p[4]);
        out_result->FaultFlags         = IG4U_RD_LE16(&p[6]);
    }

    return IG4U_CMD_OK;
}

/**
 * CG Thruster Burst TC 페이로드:
 *   [0]   ThrusterID       (uint8)
 *   [1]   Spare            (uint8)
 *   [2-3] PulseDuration_ms (uint16 LE)
 *   [4-5] PulseCount       (uint16 LE)
 *   [6-7] Interval_ms      (uint16 LE)
 */
int32_t IG4U_Cmd_CGThrusterBurst(uint8_t                         thruster_id,
                                   uint16_t                        pulse_duration_ms,
                                   uint16_t                        pulse_count,
                                   uint16_t                        interval_ms,
                                   IG4U_CGThrusterResult_Payload_t *out_result)
{
    int32_t        ret;
    uint8_t        buf[8];
    const uint8_t *p;
    uint16_t       plen;

    IG4U_PRINTF("[IG4U_CMD] >> CG BURST: ch=%u dur=%u cnt=%u interval=%u ms\n",
                thruster_id, pulse_duration_ms, pulse_count, interval_ms);

    buf[0] = thruster_id;
    buf[1] = 0U;
    IG4U_WR_LE16(&buf[2], pulse_duration_ms);
    IG4U_WR_LE16(&buf[4], pulse_count);
    IG4U_WR_LE16(&buf[6], interval_ms);

    ret = SendTC(IG4U_WIRE_MSGID_CG_BURST, buf, (uint16_t)sizeof(buf));
    if (ret != IG4U_CMD_OK) { return ret; }

    if (out_result != NULL)
    {
        ret = RecvTMRaw(IG4U_WIRE_MSGID_CG_RESULT, &p, &plen);
        if (ret != IG4U_CMD_OK) { return ret; }
        if (plen < 8U) { return IG4U_CMD_ERR_PARSE; }

        out_result->TriggerMsgID       = p[0];
        out_result->ThrusterID         = p[1];
        out_result->Result             = p[2];
        out_result->Spare              = 0U;
        out_result->ExecutedPulseCount = IG4U_RD_LE16(&p[4]);
        out_result->FaultFlags         = IG4U_RD_LE16(&p[6]);
    }

    return IG4U_CMD_OK;
}

int32_t IG4U_Cmd_CGThrusterAbort(void)
{
    IG4U_PRINTF("[IG4U_CMD] >> CG THRUSTER ABORT\n");
    return SendTC(IG4U_WIRE_MSGID_CG_ABORT, NULL, 0U);
}

/* ===================================================================
 * Group 5: Fault 관리
 * =================================================================== */

/**
 * Reply Fault Log TM 페이로드 레이아웃:
 *   [0]   EntryCount         (uint8)
 *   [1-3] Spare              (uint8 x3)
 *   이후 EntryCount개:
 *     [+0..+3] Timestamp_sec (uint32 LE)
 *     [+4..+5] FaultCode     (uint16 LE)
 *     [+6]     State_at_Fault (uint8)
 *     [+7]     Severity       (uint8)
 */
int32_t IG4U_Cmd_RequestFaultLog(IG4U_ReplyFaultLog_Payload_t *out_log)
{
    int32_t        ret;
    const uint8_t *p;
    uint16_t       plen;
    uint8_t        i;
    uint32_t       offset;

    if (out_log == NULL) { return IG4U_CMD_ERR_PARSE; }

    IG4U_PRINTF("[IG4U_CMD] >> REQUEST FAULT LOG\n");
    ret = SendTC(IG4U_WIRE_MSGID_REQUEST_FAULT_LOG, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }

    ret = RecvTMRaw(IG4U_WIRE_MSGID_REPLY_FAULT_LOG, &p, &plen);
    if (ret != IG4U_CMD_OK) { return ret; }

    if (plen < 1U) { return IG4U_CMD_ERR_PARSE; }

    out_log->EntryCount = p[0];
    if (out_log->EntryCount > IG4U_FAULT_LOG_MAX_ENTRIES)
    {
        out_log->EntryCount = IG4U_FAULT_LOG_MAX_ENTRIES;
    }

    offset = 4U;  /* EntryCount(1) + Spare(3) */
    for (i = 0U; i < out_log->EntryCount; i++)
    {
        if ((offset + 8U) > (uint32_t)plen) { break; }

        out_log->Entries[i].Timestamp_sec  = IG4U_RD_LE32(&p[offset]);
        out_log->Entries[i].FaultCode      = IG4U_RD_LE16(&p[offset + 4U]);
        out_log->Entries[i].State_at_Fault = p[offset + 6U];
        out_log->Entries[i].Severity       = p[offset + 7U];

        offset += 8U;
    }

    return IG4U_CMD_OK;
}

int32_t IG4U_Cmd_ClearFault(void)
{
    int32_t ret;
    IG4U_PRINTF("[IG4U_CMD] >> CLEAR FAULT\n");
    ret = SendTC(IG4U_WIRE_MSGID_CLEAR_FAULT, NULL, 0U);
    if (ret != IG4U_CMD_OK) { return ret; }
    return IG4U_Cmd_WaitDefaultResponse(IG4U_WIRE_MSGID_CLEAR_FAULT, NULL);
}

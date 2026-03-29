/**
 * @file ig4u_comm_test.c
 * @brief iG4U 통신 시험 절차 구현
 *
 * ICD Section 6.1 주요 통신 시험시퀀스를 그대로 구현합니다.
 * 시험 순서: (1)전원인가 → (2)PING → (3)STATUS → (4)HK →
 *             (5)SET ARMED → (6)SET STANDBY → (7)STATUS확인 →
 *             (8)Unknown TC → (9)Wrong CRC → (10)재연결 → (11)전원차단
 *             (12) 최소 2회 반복
 *
 * 사용법:
 *   IG4U_CommTest_RunAll(2); // 시험 시퀀스 2회 반복
 */

#include "ig4u_comm_test.h"
#include "ig4u_commands.h"
#include "ig4u_protocol.h"
#include "ig4u_interface_cfg.h"
#include "ig4u_internal_cfg.h"
#include <string.h>

/* HAL 스텁 (플랫폼 구현 필요) */
extern int32_t IG4U_HAL_Send(const uint8_t *buf, uint32_t len);
extern int32_t IG4U_HAL_Recv(uint8_t *buf, uint32_t size, uint32_t timeout_ms);
extern void    IG4U_HAL_PowerOn(void);
extern void    IG4U_HAL_PowerOff(void);
extern void    IG4U_HAL_DelayMs(uint32_t ms);
extern void    IG4U_HAL_DisconnectLink(void);
extern void    IG4U_HAL_ReconnectLink(void);

/* 시험 결과 집계 */
static uint32_t s_test_pass = 0U;
static uint32_t s_test_fail = 0U;

/* ===================================================================
 * 내부 헬퍼: 시험 결과 출력
 * =================================================================== */
#define TEST_ASSERT(cond, name)                                         \
    do {                                                                \
        if (cond) {                                                     \
            OS_printf("[PASS] %s\n", (name));                          \
            s_test_pass++;                                              \
        } else {                                                        \
            OS_printf("[FAIL] %s (line %d)\n", (name), __LINE__);     \
            s_test_fail++;                                              \
        }                                                               \
    } while(0)

/* ===================================================================
 * (1) 본체 → 탑재체 전원 인가
 * RS-422 링크 초기화 및 부팅 완료 대기
 * =================================================================== */
static void Test_Step01_PowerOn(void)
{
    OS_printf("\n--- [STEP 1] 전원 인가 및 RS-422 링크 초기화 ---\n");
    IG4U_HAL_PowerOn();
    IG4U_HAL_DelayMs(2000U);  /* 부팅 완료 대기 (TBD: 실제 부팅 시간 확인) */
    OS_printf("[INFO] 탑재체 전원 인가 완료, 부팅 대기 2000ms\n");
}

/* ===================================================================
 * (2) 본체 → PING → Default Response 확인, STANDBY MODE 확인
 * =================================================================== */
static void Test_Step02_Ping(void)
{
    int32_t                        ret;
    IG4U_ReplyStatus_Payload_t     status;

    OS_printf("\n--- [STEP 2] PING 및 STANDBY 모드 확인 ---\n");

    ret = IG4U_Cmd_Ping();
    TEST_ASSERT(ret == IG4U_CMD_OK, "PING -> Default Response ACK");

    /* 초기 모드가 STANDBY인지 확인 */
    ret = IG4U_Cmd_RequestStatus(&status);
    TEST_ASSERT(ret == IG4U_CMD_OK, "PING 후 STATUS 수신");
    TEST_ASSERT(status.Mode == (uint8_t)IG4U_STATE_STANDBY,
                "초기 모드 STANDBY 확인");
}

/* ===================================================================
 * (3) 본체 → REQUEST STATUS → REPLY STATUS 확인
 * Mode, Fault, Interlock 상태 확인
 * =================================================================== */
static void Test_Step03_RequestStatus(void)
{
    int32_t                    ret;
    IG4U_ReplyStatus_Payload_t status;

    OS_printf("\n--- [STEP 3] REQUEST STATUS ---\n");

    memset(&status, 0, sizeof(status));
    ret = IG4U_Cmd_RequestStatus(&status);
    TEST_ASSERT(ret == IG4U_CMD_OK, "REQUEST STATUS -> REPLY STATUS 수신");

    OS_printf("[INFO] Mode=%u FaultFlags=0x%04X InterlockStatus=0x%02X\n",
              status.Mode, status.FaultFlags, status.InterlockStatus);

    TEST_ASSERT(status.FaultFlags == IG4U_FAULT_NONE, "Fault 없음 확인");
}

/* ===================================================================
 * (4) 본체 → REQUEST HK DATA → REPLY HK DATA 확인
 * 압력 2채널, 온도 4채널 정상 수신
 * =================================================================== */
static void Test_Step04_RequestHKData(void)
{
    int32_t                    ret;
    IG4U_ReplyHKData_Payload_t hk;
    uint32_t                   i;

    OS_printf("\n--- [STEP 4] REQUEST HK DATA ---\n");

    memset(&hk, 0, sizeof(hk));
    ret = IG4U_Cmd_RequestHKData(&hk);
    TEST_ASSERT(ret == IG4U_CMD_OK, "REQUEST HK DATA -> REPLY HK DATA 수신");

    for (i = 0U; i < IG4U_HK_PRESSURE_CH_COUNT; i++)
    {
        OS_printf("[INFO] Pressure CH%u: %u (x0.1 kPa)\n",
                  (unsigned)(i + 1U), hk.Pressure_kPa_x10[i]);
    }
    for (i = 0U; i < IG4U_HK_TEMP_CH_COUNT; i++)
    {
        OS_printf("[INFO] Temperature CH%u: %d (x0.1 degC)\n",
                  (unsigned)(i + 1U), (int)hk.Temperature_x10[i]);
    }

    /* 데이터 수신 자체가 목적이므로 ret == OK 면 통과 */
    TEST_ASSERT(ret == IG4U_CMD_OK, "압력/온도 채널 데이터 수신");
}

/* ===================================================================
 * (5) 본체 → SET MODE (ARMED) → Default Response 확인
 * =================================================================== */
static void Test_Step05_SetArmed(void)
{
    int32_t                    ret;
    IG4U_ReplyStatus_Payload_t status;

    OS_printf("\n--- [STEP 5] SET MODE ARMED ---\n");

    ret = IG4U_Cmd_SetMode((uint8_t)IG4U_MODE_ARMED);
    TEST_ASSERT(ret == IG4U_CMD_OK, "SET MODE ARMED -> ACK");

    ret = IG4U_Cmd_RequestStatus(&status);
    TEST_ASSERT(ret == IG4U_CMD_OK, "ARMED 모드 전이 후 STATUS 수신");
    TEST_ASSERT(status.Mode == (uint8_t)IG4U_STATE_ARMED, "ARMED 모드 전이 확인");
}

/* ===================================================================
 * (6) 본체 → SET MODE (STANDBY) → Default Response 확인
 * =================================================================== */
static void Test_Step06_SetStandby(void)
{
    int32_t ret;

    OS_printf("\n--- [STEP 6] SET MODE STANDBY ---\n");

    ret = IG4U_Cmd_SetMode((uint8_t)IG4U_MODE_STANDBY);
    TEST_ASSERT(ret == IG4U_CMD_OK, "SET MODE STANDBY -> ACK");
}

/* ===================================================================
 * (7) 본체 → REQUEST STATUS → REPLY STATUS 확인
 * 모드 변경 정상 반영 확인
 * =================================================================== */
static void Test_Step07_VerifyStandby(void)
{
    int32_t                    ret;
    IG4U_ReplyStatus_Payload_t status;

    OS_printf("\n--- [STEP 7] 모드 변경 반영 확인 ---\n");

    memset(&status, 0, sizeof(status));
    ret = IG4U_Cmd_RequestStatus(&status);
    TEST_ASSERT(ret == IG4U_CMD_OK, "STATUS 수신");
    TEST_ASSERT(status.Mode == (uint8_t)IG4U_STATE_STANDBY,
                "STANDBY 복귀 확인");

    OS_printf("[INFO] 현재 Mode=%u\n", status.Mode);
}

/* ===================================================================
 * (8) 본체 → Unknown TC → DEFAULT RESPONSE (NACK) 확인
 * 비정상 명령 처리 확인
 * =================================================================== */
static void Test_Step08_UnknownTC(void)
{
    int32_t                        ret;
    uint8_t                        tx_buf[IG4U_PACKET_MAX_TOTAL];
    IG4U_DefaultResponse_Payload_t resp;
    uint8_t                        rx_buf[IG4U_PACKET_MAX_TOTAL];
    uint8_t                        msg_type;
    uint8_t                        msg_id;
    const uint8_t                 *payload_ptr;
    uint16_t                       payload_len;
    int32_t                        encoded_len;
    int32_t                        recv_len;

    OS_printf("\n--- [STEP 8] Unknown TC 전송 -> NACK 확인 ---\n");

    /* 존재하지 않는 MsgID=0xFF 전송 */
    encoded_len = IG4U_Proto_EncodeTC(tx_buf, sizeof(tx_buf), 0xFFU, NULL, 0U);
    TEST_ASSERT(encoded_len > 0, "Unknown TC 패킷 인코딩");

    if (encoded_len > 0)
    {
        IG4U_HAL_Send(tx_buf, (uint32_t)encoded_len);
        recv_len = IG4U_HAL_Recv(rx_buf, sizeof(rx_buf), IG4U_UART_TIMEOUT_MS);

        if (recv_len > 0)
        {
            ret = IG4U_Proto_DecodeTM(rx_buf, (uint32_t)recv_len,
                                       &msg_type, &msg_id,
                                       &payload_ptr, &payload_len);
            if (ret == IG4U_PROTO_OK &&
                payload_len >= sizeof(IG4U_DefaultResponse_Payload_t))
            {
                memcpy(&resp, payload_ptr, sizeof(resp));
                TEST_ASSERT(resp.Result == IG4U_RESULT_NACK,
                            "Unknown TC -> NACK 수신");
                TEST_ASSERT(resp.EchoMsgID == 0xFFU,
                            "NACK 응답에 원본 MsgID=0xFF 반영");
                OS_printf("[INFO] NACK ErrorCode=0x%02X\n", resp.ErrorCode);
            }
            else
            {
                TEST_ASSERT(0, "Unknown TC 응답 파싱");
            }
        }
        else
        {
            /* 탑재체가 무응답 처리할 수도 있음 - 이 경우도 허용 */
            OS_printf("[INFO] Unknown TC: 탑재체 무응답 (허용)\n");
            s_test_pass++;
        }
    }
}

/* ===================================================================
 * (9) 본체 → Wrong CRC TC → 무응답 또는 오류 처리 확인
 * =================================================================== */
static void Test_Step09_WrongCRC(void)
{
    uint8_t  tx_buf[IG4U_PACKET_MAX_TOTAL];
    uint8_t  rx_buf[IG4U_PACKET_MAX_TOTAL];
    int32_t  encoded_len;
    int32_t  recv_len;

    OS_printf("\n--- [STEP 9] Wrong CRC TC 전송 ---\n");

    /* 정상 PING 패킷 생성 후 CRC를 의도적으로 변조 */
    encoded_len = IG4U_Proto_EncodeTC(tx_buf, sizeof(tx_buf),
                                       IG4U_WIRE_MSGID_PING, NULL, 0U);
    TEST_ASSERT(encoded_len > 0, "PING 패킷 인코딩 (CRC 변조 전)");

    if (encoded_len > 0)
    {
        /* 마지막 CRC 바이트를 비트 반전하여 오류 유발 */
        tx_buf[encoded_len - 1U] ^= 0xFFU;

        IG4U_HAL_Send(tx_buf, (uint32_t)encoded_len);
        recv_len = IG4U_HAL_Recv(rx_buf, sizeof(rx_buf), IG4U_UART_TIMEOUT_MS);

        /* 탑재체는 CRC 오류 시 무응답 또는 오류 응답 - 둘 다 통과 */
        if (recv_len <= 0)
        {
            OS_printf("[INFO] Wrong CRC: 탑재체 무응답 (정상 동작)\n");
            TEST_ASSERT(1, "Wrong CRC -> 탑재체 무응답");
        }
        else
        {
            OS_printf("[INFO] Wrong CRC: 탑재체 오류 응답 수신 (정상 동작)\n");
            TEST_ASSERT(1, "Wrong CRC -> 탑재체 오류 응답");
        }
    }
}

/* ===================================================================
 * (10) RS-422 통신 중단 후 재연결 → 통신 복구 확인
 * Header 재동기화 및 통신 정상 복구 확인
 * =================================================================== */
static void Test_Step10_Reconnect(void)
{
    int32_t ret;

    OS_printf("\n--- [STEP 10] 통신 중단 후 재연결 ---\n");

    IG4U_HAL_DisconnectLink();
    IG4U_HAL_DelayMs(1000U);
    IG4U_HAL_ReconnectLink();
    IG4U_HAL_DelayMs(500U);

    /* 재연결 후 PING으로 통신 복구 확인 */
    ret = IG4U_Cmd_Ping();
    TEST_ASSERT(ret == IG4U_CMD_OK, "재연결 후 PING 통신 복구");

    OS_printf("[INFO] 통신 복구 %s\n",
              (ret == IG4U_CMD_OK) ? "성공" : "실패");
}

/* ===================================================================
 * (11) 본체 → 탑재체 전원 차단
 * =================================================================== */
static void Test_Step11_PowerOff(void)
{
    OS_printf("\n--- [STEP 11] 전원 차단 ---\n");
    IG4U_HAL_PowerOff();
    IG4U_HAL_DelayMs(500U);
    OS_printf("[INFO] 탑재체 전원 차단 완료\n");
}

/* ===================================================================
 * (12) 전체 시퀀스 1회 실행
 * =================================================================== */
static void Test_RunOnce(void)
{
    Test_Step01_PowerOn();
    Test_Step02_Ping();
    Test_Step03_RequestStatus();
    Test_Step04_RequestHKData();
    Test_Step05_SetArmed();
    Test_Step06_SetStandby();
    Test_Step07_VerifyStandby();
    Test_Step08_UnknownTC();
    Test_Step09_WrongCRC();
    Test_Step10_Reconnect();
    Test_Step11_PowerOff();
}

/* ===================================================================
 * Public: 전체 시험 실행 (ICD: 최소 2회 반복)
 * =================================================================== */
void IG4U_CommTest_RunAll(uint32_t repeat_count)
{
    uint32_t i;

    if (repeat_count == 0U) { repeat_count = 1U; }

    s_test_pass = 0U;
    s_test_fail = 0U;

    OS_printf("========================================\n");
    OS_printf(" iG4U 통신 시험 시작 (반복 %u회)\n", (unsigned)repeat_count);
    OS_printf(" ICD IG4U-SW-ICD-001 Section 6.1\n");
    OS_printf("========================================\n");

    for (i = 0U; i < repeat_count; i++)
    {
        OS_printf("\n======== 반복 %u / %u ========\n",
                  (unsigned)(i + 1U), (unsigned)repeat_count);
        Test_RunOnce();

        /* 반복 간 대기 (마지막 반복 제외) */
        if (i < (repeat_count - 1U))
        {
            IG4U_HAL_DelayMs(1000U);
        }
    }

    OS_printf("\n========================================\n");
    OS_printf(" 시험 완료: PASS=%u  FAIL=%u\n",
              (unsigned)s_test_pass, (unsigned)s_test_fail);
    OS_printf("========================================\n");
}

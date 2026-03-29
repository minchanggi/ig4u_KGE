/**
 * @file ig4u_hal.c
 * @brief iG4U RS-422 HAL 구현 (CFE_SRL API 사용)
 *
 * ============================================================
 * RS-422 UART 설정 (ICD Section 3.2)
 *   Baud  : 921,600 bps (TBD)
 *   Data  : 8 bits
 *   Parity: None
 *   Stop  : 1 bit
 *   Order : LSB First
 * ============================================================
 *
 * CFE_SRL 사용 흐름:
 *   1. CFE_SRL_ApiGetHandle()  : UART 핸들 획득
 *   2. CFE_SRL_ApiWrite()      : 패킷 송신
 *   3. CFE_SRL_ApiRead()       : 응답 수신 (타임아웃 포함)
 *
 * 수신 전략:
 *   - 먼저 최소 헤더 크기(IG4U_PACKET_OVERHEAD_BYTES)만큼 읽어
 *     Length 필드를 파악한 후, 나머지 페이로드+CRC를 추가로 읽습니다.
 *   - 이 방식으로 단일 CFE_SRL_ApiRead() 호출로 가변 길이 패킷을
 *     안정적으로 수신합니다.
 */

#include "ig4u_hal.h"
#include "ig4u_protocol.h"   /* IG4U_Proto_FindSyncHeader() */
#include "ig4u_internal_cfg.h"

/* CFE SRL 드라이버 헤더 (OBC 플랫폼 제공) */
#include "cfe_srl_api.h"

#include <string.h>

/* ===================================================================
 * 내부 상태
 * =================================================================== */
static CFE_SRL_Handle_t s_uart_handle;
static bool             s_initialized = false;

/* IG4U_PACKET_OVERHEAD_BYTES = Header(2)+Ver(1)+MsgType(1)+MsgID(1)+Len(2)+CRC(2) = 9 */
#define HAL_HEADER_READ_SIZE    IG4U_PACKET_OVERHEAD_BYTES

/* ===================================================================
 * 초기화
 * =================================================================== */
int32_t IG4U_HAL_Init(void)
{
    CFE_Status_t      status;
    CFE_SRL_Config_t  cfg;

    if (s_initialized)
    {
        return IG4U_HAL_OK;  /* 중복 초기화 방지 */
    }

    /* UART 핸들 획득 (PAYLGBAT의 CFE_SRL_ApiGetHandle 패턴 동일) */
    status = CFE_SRL_ApiGetHandle(IG4U_UART_HANDLE_INDEXER, &s_uart_handle);
    if (status != CFE_SUCCESS)
    {
        IG4U_PRINTF("[IG4U_HAL] GetHandle failed. RC=0x%08lX\n",
                    (unsigned long)status);
        return IG4U_HAL_ERR_INIT;
    }

    /* UART 파라미터 설정 (ICD Section 3.2) */
    memset(&cfg, 0, sizeof(cfg));
    cfg.BaudRate  = IG4U_UART_BAUD_RATE;   /* 921600 bps  */
    cfg.DataBits  = IG4U_UART_DATA_BITS;   /* 8           */
    cfg.Parity    = IG4U_UART_PARITY;      /* None (0)    */
    cfg.StopBits  = IG4U_UART_STOP_BITS;   /* 1           */

    status = CFE_SRL_ApiConfigure(s_uart_handle, &cfg);
    if (status != CFE_SUCCESS)
    {
        IG4U_PRINTF("[IG4U_HAL] Configure failed. RC=0x%08lX\n",
                    (unsigned long)status);
        return IG4U_HAL_ERR_INIT;
    }

    s_initialized = true;
    IG4U_PRINTF("[IG4U_HAL] RS-422 init OK. Baud=%u 8N1 LSB-first\n",
                IG4U_UART_BAUD_RATE);
    return IG4U_HAL_OK;
}

/* ===================================================================
 * 해제
 * =================================================================== */
void IG4U_HAL_Deinit(void)
{
    if (s_initialized)
    {
        CFE_SRL_ApiReleaseHandle(s_uart_handle);
        s_initialized = false;
        IG4U_PRINTF("[IG4U_HAL] RS-422 released.\n");
    }
}

/* ===================================================================
 * 송신
 * =================================================================== */
int32_t IG4U_HAL_Send(const uint8_t *buf, uint32_t len)
{
    CFE_Status_t status;
    uint32_t     bytes_written = 0;

    if (!s_initialized)
    {
        return IG4U_HAL_ERR_NOT_INIT;
    }
    if (buf == NULL || len == 0U)
    {
        return IG4U_HAL_ERR_SEND;
    }

    /*
     * CFE_SRL_ApiWrite(handle, tx_buf, tx_len, &bytes_written, timeout_ms)
     * PAYLGBAT에서 I2C Write-then-Read 방식으로 CFE_SRL_ApiRead를 쓰는 것과
     * 동일한 API 패턴. UART는 Write 단독 호출.
     */
    status = CFE_SRL_ApiWrite(s_uart_handle,
                               (void *)buf,
                               len,
                               &bytes_written,
                               IG4U_UART_TIMEOUT_MS);

    if (status != CFE_SUCCESS || bytes_written != len)
    {
        IG4U_PRINTF("[IG4U_HAL] Send failed. RC=0x%08lX written=%u expected=%u\n",
                    (unsigned long)status, (unsigned)bytes_written, (unsigned)len);
        return IG4U_HAL_ERR_SEND;
    }

    IG4U_PRINTF("[IG4U_HAL] Sent %u bytes.\n", (unsigned)len);
    return IG4U_HAL_OK;
}

/* ===================================================================
 * 수신 (가변 길이 패킷 처리)
 *
 * 수신 전략:
 *   Step 1. OVERHEAD 크기만큼 읽어 Length 필드 파악
 *   Step 2. 남은 payload + CRC(2) 추가 수신
 *   Step 3. 헤더 동기화 실패 시 FindSyncHeader()로 복구
 * =================================================================== */
int32_t IG4U_HAL_Recv(uint8_t *buf, uint32_t size, uint32_t timeout_ms)
{
    CFE_Status_t status;
    uint32_t     bytes_read  = 0;
    uint16_t     payload_len = 0;
    uint32_t     remain      = 0;
    int32_t      sync_offset = 0;

    if (!s_initialized)
    {
        return IG4U_HAL_ERR_NOT_INIT;
    }
    if (buf == NULL || size < IG4U_PACKET_OVERHEAD_BYTES)
    {
        return IG4U_HAL_ERR_RECV;
    }

    /* ------------------------------------------------------------------
     * Step 1: 헤더 + 고정 필드(OVERHEAD) 수신
     * ------------------------------------------------------------------ */
    status = CFE_SRL_ApiRead(s_uart_handle,
                              (void *)buf,
                              HAL_HEADER_READ_SIZE,
                              &bytes_read,
                              timeout_ms);

    if (status != CFE_SUCCESS || bytes_read == 0U)
    {
        IG4U_PRINTF("[IG4U_HAL] Recv header timeout. RC=0x%08lX\n",
                    (unsigned long)status);
        return IG4U_HAL_ERR_TIMEOUT;
    }

    /* ------------------------------------------------------------------
     * Step 2: 헤더 동기화 확인 (0xAA55)
     * 쓰레기 데이터가 앞에 있을 경우 동기 위치를 탐색하여 재정렬
     * ICD Section 6.1 (10)번 시험 절차 대응
     * ------------------------------------------------------------------ */
    sync_offset = IG4U_Proto_FindSyncHeader(buf, bytes_read);
    if (sync_offset < 0)
    {
        IG4U_PRINTF("[IG4U_HAL] Sync header 0xAA55 not found. Flushing.\n");
        IG4U_HAL_FlushRx();
        return IG4U_HAL_ERR_RECV;
    }

    if (sync_offset > 0)
    {
        /* 동기 위치가 0이 아니면 앞쪽 쓰레기 데이터를 버리고 재정렬 */
        IG4U_PRINTF("[IG4U_HAL] Sync at offset %d. Realigning.\n", (int)sync_offset);
        memmove(buf, buf + sync_offset, bytes_read - (uint32_t)sync_offset);
        bytes_read -= (uint32_t)sync_offset;

        /* 부족한 만큼 추가 수신 */
        remain = HAL_HEADER_READ_SIZE - bytes_read;
        if (remain > 0U)
        {
            uint32_t extra = 0;
            status = CFE_SRL_ApiRead(s_uart_handle,
                                      (void *)(buf + bytes_read),
                                      remain,
                                      &extra,
                                      timeout_ms);
            if (status != CFE_SUCCESS)
            {
                return IG4U_HAL_ERR_TIMEOUT;
            }
            bytes_read += extra;
        }
    }

    /* ------------------------------------------------------------------
     * Step 3: Length 필드 파악 후 페이로드 + CRC 추가 수신
     * 패킷 구조: [0-1]=0xAA55 [2]=Ver [3]=MsgType [4]=MsgID [5-6]=Length ...
     * Length는 Byte[5:6], Little-Endian
     * ------------------------------------------------------------------ */
    if (bytes_read < HAL_HEADER_READ_SIZE)
    {
        return IG4U_HAL_ERR_RECV;
    }

    payload_len = (uint16_t)buf[5] | (uint16_t)((uint16_t)buf[6] << 8U);

    if (payload_len > IG4U_PACKET_MAX_PAYLOAD)
    {
        IG4U_PRINTF("[IG4U_HAL] Payload length too large: %u\n", payload_len);
        IG4U_HAL_FlushRx();
        return IG4U_HAL_ERR_RECV;
    }

    /* 페이로드(N) + CRC(2) 추가 수신 */
    remain = (uint32_t)payload_len + 2U;  /* +2 = CRC16 */

    if ((bytes_read + remain) > size)
    {
        IG4U_PRINTF("[IG4U_HAL] Buffer too small. need=%u have=%u\n",
                    (unsigned)(bytes_read + remain), (unsigned)size);
        return IG4U_HAL_ERR_RECV;
    }

    if (remain > 0U)
    {
        uint32_t extra = 0;
        status = CFE_SRL_ApiRead(s_uart_handle,
                                  (void *)(buf + bytes_read),
                                  remain,
                                  &extra,
                                  timeout_ms);
        if (status != CFE_SUCCESS || extra != remain)
        {
            IG4U_PRINTF("[IG4U_HAL] Recv payload timeout. got=%u exp=%u\n",
                        (unsigned)extra, (unsigned)remain);
            return IG4U_HAL_ERR_TIMEOUT;
        }
        bytes_read += extra;
    }

    IG4U_PRINTF("[IG4U_HAL] Recv %u bytes. MsgID=0x%02X PayloadLen=%u\n",
                (unsigned)bytes_read, buf[4], payload_len);

    return (int32_t)bytes_read;
}

/* ===================================================================
 * 수신 버퍼 플러시
 * 재연결 또는 CRC 오류 후 잔여 데이터 제거
 * =================================================================== */
void IG4U_HAL_FlushRx(void)
{
    uint8_t  dummy[64];
    uint32_t bytes_read = 0;

    if (!s_initialized) { return; }

    /* 짧은 타임아웃으로 빠르게 비움 */
    while (CFE_SRL_ApiRead(s_uart_handle,
                            (void *)dummy,
                            sizeof(dummy),
                            &bytes_read,
                            10U) == CFE_SUCCESS && bytes_read > 0U)
    {
        IG4U_PRINTF("[IG4U_HAL] Flush: discarded %u bytes.\n", (unsigned)bytes_read);
    }
}

/* ===================================================================
 * 상태 확인
 * =================================================================== */
bool IG4U_HAL_IsReady(void)
{
    return s_initialized;
}

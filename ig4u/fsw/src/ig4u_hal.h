#ifndef IG4U_HAL_H
#define IG4U_HAL_H

/**
 * @file ig4u_hal.h
 * @brief iG4U RS-422 하드웨어 추상화 레이어
 *
 * CFE_SRL API를 사용하여 실제 RS-422/UART 송수신을 구현합니다.
 *
 * 참조: IG4U-SW-ICD-001 Section 3
 *   - 물리 계층: RS-422 차동 신호
 *   - Baud: 921,600 bps (TBD)
 *   - 8N1, LSB First
 *
 * CFE_SRL API 사용 패턴 (PAYLGBAT I2C 참조):
 *   CFE_SRL_ApiGetHandle()  → 핸들 획득
 *   CFE_SRL_ApiWrite()      → 송신
 *   CFE_SRL_ApiRead()       → 수신
 */

#include "common_types.h"
#include "cfe.h"
#include "ig4u_interface_cfg.h"

/* 반환 코드 */
#define IG4U_HAL_OK              0
#define IG4U_HAL_ERR_INIT       -1
#define IG4U_HAL_ERR_SEND       -2
#define IG4U_HAL_ERR_RECV       -3
#define IG4U_HAL_ERR_TIMEOUT    -4
#define IG4U_HAL_ERR_NOT_INIT   -5

/**
 * @brief RS-422 링크 초기화
 *
 * CFE_SRL 핸들을 획득하고 UART 파라미터를 설정합니다.
 * IG4U_Init() 에서 한 번 호출합니다.
 *
 * @return IG4U_HAL_OK 또는 오류 코드
 */
int32_t IG4U_HAL_Init(void);

/**
 * @brief RS-422 링크 해제
 */
void IG4U_HAL_Deinit(void);

/**
 * @brief RS-422 송신
 *
 * @param buf  송신 버퍼
 * @param len  송신 바이트 수
 * @return IG4U_HAL_OK 또는 오류 코드
 */
int32_t IG4U_HAL_Send(const uint8_t *buf, uint32_t len);

/**
 * @brief RS-422 수신 (타임아웃 대기)
 *
 * timeout_ms 내에 수신된 바이트 수를 반환합니다.
 * 헤더(0xAA55) 동기화는 호출자(protocol 레이어)에서 처리합니다.
 *
 * @param buf        수신 버퍼
 * @param size       버퍼 크기
 * @param timeout_ms 타임아웃 (ms)
 * @return 수신 바이트 수 (양수), 타임아웃/오류 시 음수
 */
int32_t IG4U_HAL_Recv(uint8_t *buf, uint32_t size, uint32_t timeout_ms);

/**
 * @brief 수신 버퍼 플러시 (쓰레기 데이터 제거)
 */
void IG4U_HAL_FlushRx(void);

/**
 * @brief 링크 초기화 여부 확인
 */
bool IG4U_HAL_IsReady(void);

#endif /* IG4U_HAL_H */

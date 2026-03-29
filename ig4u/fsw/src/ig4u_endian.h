#ifndef IG4U_ENDIAN_H
#define IG4U_ENDIAN_H

/**
 * @file ig4u_endian.h
 * @brief iG4U 엔디안 변환 매크로
 *
 * ICD Section 3.2 "Sending Order: LSB first" → Little-Endian 가정.
 * iG4U MCU 엔디안이 확정되면 IG4U_WIRE_BIG_ENDIAN 정의로 전환 가능.
 *
 * 사용법:
 *   송신(TC) : HOST_TO_LE16(val) → wire 바이트열로 변환
 *   수신(TM) : IG4U_RD_LE16(ptr) → wire 바이트열에서 host 값 추출
 *              IG4U_RD_LE16S(ptr) → signed int16 버전
 */

#include "common_types.h"

/* ===================================================================
 * 와이어 엔디안 선택
 * Little-Endian : #undef  IG4U_WIRE_BIG_ENDIAN  (기본값)
 * Big-Endian    : #define IG4U_WIRE_BIG_ENDIAN
 * =================================================================== */
/* #define IG4U_WIRE_BIG_ENDIAN */

/* ===================================================================
 * 바이트열 → host 값 (TM 수신 파싱용)
 * ptr : uint8_t* (wire buffer의 해당 오프셋)
 * =================================================================== */
#ifndef IG4U_WIRE_BIG_ENDIAN

/* Little-Endian wire */
#define IG4U_RD_LE16(ptr) \
    ((uint16_t)( (uint16_t)(((const uint8_t*)(ptr))[0])        \
               | (uint16_t)(((const uint8_t*)(ptr))[1]) << 8U ))

#define IG4U_RD_LE16S(ptr) \
    ((int16_t)IG4U_RD_LE16(ptr))

#define IG4U_RD_LE32(ptr) \
    ((uint32_t)( (uint32_t)(((const uint8_t*)(ptr))[0])         \
               | (uint32_t)(((const uint8_t*)(ptr))[1]) <<  8U  \
               | (uint32_t)(((const uint8_t*)(ptr))[2]) << 16U  \
               | (uint32_t)(((const uint8_t*)(ptr))[3]) << 24U ))

#else

/* Big-Endian wire */
#define IG4U_RD_LE16(ptr) \
    ((uint16_t)( (uint16_t)(((const uint8_t*)(ptr))[0]) << 8U  \
               | (uint16_t)(((const uint8_t*)(ptr))[1])        ))

#define IG4U_RD_LE16S(ptr) \
    ((int16_t)IG4U_RD_LE16(ptr))

#define IG4U_RD_LE32(ptr) \
    ((uint32_t)( (uint32_t)(((const uint8_t*)(ptr))[0]) << 24U \
               | (uint32_t)(((const uint8_t*)(ptr))[1]) << 16U \
               | (uint32_t)(((const uint8_t*)(ptr))[2]) <<  8U \
               | (uint32_t)(((const uint8_t*)(ptr))[3])        ))

#endif /* IG4U_WIRE_BIG_ENDIAN */

/* ===================================================================
 * host 값 → 바이트열 (TC 송신 직렬화용)
 * buf : uint8_t* (쓸 위치), val : 변환할 값
 * =================================================================== */
#ifndef IG4U_WIRE_BIG_ENDIAN

/* Little-Endian wire */
#define IG4U_WR_LE16(buf, val) \
    do { \
        ((uint8_t*)(buf))[0] = (uint8_t)((uint16_t)(val) & 0xFFU);        \
        ((uint8_t*)(buf))[1] = (uint8_t)(((uint16_t)(val) >> 8U) & 0xFFU); \
    } while(0)

#define IG4U_WR_LE32(buf, val) \
    do { \
        ((uint8_t*)(buf))[0] = (uint8_t)((uint32_t)(val) & 0xFFU);         \
        ((uint8_t*)(buf))[1] = (uint8_t)(((uint32_t)(val) >>  8U) & 0xFFU); \
        ((uint8_t*)(buf))[2] = (uint8_t)(((uint32_t)(val) >> 16U) & 0xFFU); \
        ((uint8_t*)(buf))[3] = (uint8_t)(((uint32_t)(val) >> 24U) & 0xFFU); \
    } while(0)

#else

/* Big-Endian wire */
#define IG4U_WR_LE16(buf, val) \
    do { \
        ((uint8_t*)(buf))[0] = (uint8_t)(((uint16_t)(val) >> 8U) & 0xFFU); \
        ((uint8_t*)(buf))[1] = (uint8_t)((uint16_t)(val) & 0xFFU);         \
    } while(0)

#define IG4U_WR_LE32(buf, val) \
    do { \
        ((uint8_t*)(buf))[0] = (uint8_t)(((uint32_t)(val) >> 24U) & 0xFFU); \
        ((uint8_t*)(buf))[1] = (uint8_t)(((uint32_t)(val) >> 16U) & 0xFFU); \
        ((uint8_t*)(buf))[2] = (uint8_t)(((uint32_t)(val) >>  8U) & 0xFFU); \
        ((uint8_t*)(buf))[3] = (uint8_t)((uint32_t)(val) & 0xFFU);          \
    } while(0)

#endif /* IG4U_WIRE_BIG_ENDIAN */

#endif /* IG4U_ENDIAN_H */

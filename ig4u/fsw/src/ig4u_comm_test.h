#ifndef IG4U_COMM_TEST_H
#define IG4U_COMM_TEST_H

/**
 * @file ig4u_comm_test.h
 * @brief iG4U 통신 시험 절차 (ICD Section 6.1)
 */

#include "common_types.h"

/**
 * @brief 통신 시험 시퀀스 실행
 * @param repeat_count 반복 횟수 (ICD: 최소 2회)
 */
void IG4U_CommTest_RunAll(uint32_t repeat_count);

#endif /* IG4U_COMM_TEST_H */

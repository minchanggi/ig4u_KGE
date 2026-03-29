/**
 * @file ig4u_dispatch.h
 * @brief iG4U cFS 메시지 디스패치
 */
#ifndef IG4U_DISPATCH_H
#define IG4U_DISPATCH_H

#include "cfe.h"
#include "common_types.h"

bool IG4U_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);
void IG4U_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr);
void IG4U_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr);

#endif /* IG4U_DISPATCH_H */

#pragma once


#include <initguid.h>
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Common.h"

// ���������� �����
#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

// ���� �����������
#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

// ���� ������ ��������� �� �������
extern ULONG gTraceFlags;

#define MAKEULONG(A,B,C,D) (ULONG) (A << 8) | (B << 16) | (C << 24) | D

// ��� �������� ������ ��� ���� ���������
#define MEM_TAG_OBJECT_NAME MAKEULONG('O', 'B', 'J', 'N')
#define MEM_TAG_PROC_NAME MAKEULONG('P', 'R', 'O', 'C')
#define MEM_TAG_VOLUME_NAME MAKEULONG('V', 'O', 'L', 'N')
#define MEM_TAG_STRFILTER_NAME MAKEULONG('S', 'F', 'L', 'T')
#define MEM_TAG_INPUT_FNAME MAKEULONG('I', 'N', 'F', 'N')
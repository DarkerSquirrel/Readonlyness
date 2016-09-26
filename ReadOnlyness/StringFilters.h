#pragma once

#include "CommonKernel.h"
#include "Helper.h"

/// ��������� ������ ������ ��������� �������� ��� ������ ����������
DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) typedef struct _SSTRING_FILTER_ITEM
{
	PVOID Next;
	// ����� ��� �������� �� ����������
	PUNICODE_STRING Mask;
} SSTRING_FILTER_ITEM;

typedef SSTRING_FILTER_ITEM * PSSTRING_FILTER_ITEM;

/// ������������� ������ ������-�����
void InitStringFilters();

/// ���������� ������-�������
NTSTATUS AddStringFilter(PCHAR FilterString);

/// ������� ���� ��������
void ClearStringFilters();

/// ��������������� ������ ������-�����
void DeinitStringFilters();

/// ����� � �����������
BOOLEAN MatchInStringFilters(PUNICODE_STRING FileName);

/// ��������� ���� ����� �� �������
BOOLEAN WildTextCompare(
	PWCH pTameText,   // A string without wildcards
	PWCH pWildText    // A (potentially) corresponding string with wildcards
);
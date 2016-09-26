#pragma once

#define READONLYNESS_PORT_NAME L"\\READONLYNESS"

// ���� ������
typedef enum _ROCommands
{
	FlushRules = 1,
	AddRule = 2
} ROCommands;

// ��������� �������
typedef struct _S_ROCOMMAND
{
	// ��� �������
	ROCommands Command;
	// ����� ������ ������� (������� ���������� ��� ANSI CHAR)
	USHORT RuleLength;
} S_ROCOMMAND, *PS_ROCOMMAND;
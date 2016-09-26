#include "StringFilters.h"

// ������ ������� ������
volatile PSSTRING_FILTER_ITEM ListStart = NULL;
// ��������� ������� ������
volatile PSSTRING_FILTER_ITEM ListEnd = NULL;
// ������� ��� ������������� ������� � �������
KSPIN_LOCK filtersSpinLock;

/// �������������� ������ ��������
void InitStringFilters()
{
	// ������������� ������������� �������
	KeInitializeSpinLock(&filtersSpinLock);
}

/// ���������� ������-������� (C:\Program 
NTSTATUS AddStringFilter(PCHAR FilterString)
{
	// ������ ��������
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	// ������ ������ ANSI
	ANSI_STRING AnsiStr;

	// ����� ����� UNICODE ������
	PUNICODE_STRING pUniStr = NULL;

	// ����� ����� �������������� ������������� ������ ����� �����
	PUNICODE_STRING DosLink = NULL;

	// ������ ������ ��� ��������������
	USHORT UniStrBuffSize = 0;

	if ((FilterString[0] >= 'A' && FilterString[0] <= 'Z') || (FilterString[0] >= 'a' && FilterString[0] <= 'z'))
	{
		WCHAR DosDiskLetter = (WCHAR)FilterString[0];
		DosDiskLetter = RtlUpcaseUnicodeChar(DosDiskLetter);

		if (FilterString[1] == ':')
		{
			// ������ ������ - ����� �����, ��������������
			ntStatus = GetDosDeviceName(DosDiskLetter, &DosLink);

			if (!NT_SUCCESS(ntStatus))
			{
				return ntStatus;
			}

			UniStrBuffSize = DosLink->Length;
		}
	}

	try
	{
		// ���� ���������� ����� �����?
		if (DosLink != NULL)
		{
			// ��, �� ��������� ��
			RtlInitAnsiString(&AnsiStr, &FilterString[2]);
		}
		else
		{
			// ������ ������� ��� ����� �����
			RtlInitAnsiString(&AnsiStr, FilterString);
		}

		UniStrBuffSize += (USHORT)RtlAnsiStringToUnicodeSize(&AnsiStr);

		// ������ �� ������ ��������
		if (UniStrBuffSize <= sizeof(UNICODE_NULL))
		{
			return ntStatus;
		}

		pUniStr = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), MEM_TAG_STRFILTER_NAME);

		if (pUniStr != NULL)
		{
			pUniStr->Buffer = ExAllocatePoolWithTag(NonPagedPool, UniStrBuffSize + sizeof(UNICODE_NULL), MEM_TAG_STRFILTER_NAME);

			if (pUniStr->Buffer != NULL)
			{
				UNICODE_STRING convertedStr;
				RtlZeroMemory(&convertedStr, sizeof(UNICODE_STRING));

				// ������������� ������� ������
				pUniStr->Length = 0;
				pUniStr->MaximumLength = UniStrBuffSize + sizeof(UNICODE_NULL);

				// �������� �������������� ������ ����� ����� ��� �������������
				if (DosLink != NULL)
				{
					ntStatus = RtlAppendUnicodeStringToString(pUniStr, DosLink);
				}
				else
				{
					ntStatus = STATUS_SUCCESS;
				}

				if (NT_SUCCESS(ntStatus))
				{
					// ������������ ������ � ������
					ntStatus = RtlAnsiStringToUnicodeString(&convertedStr, &AnsiStr, TRUE);

					if (NT_SUCCESS(ntStatus))
					{
						// ��������� � ��������������� ����� �����
						ntStatus = RtlAppendUnicodeStringToString(pUniStr, &convertedStr);

						if (NT_SUCCESS(ntStatus))
						{
							// ������� ������ ������ � ��������� ���
							PSSTRING_FILTER_ITEM NewFilterItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(SSTRING_FILTER_ITEM), MEM_TAG_STRFILTER_NAME);

							if (NewFilterItem != NULL)
							{
								KIRQL OldIrql;

								RtlZeroMemory(NewFilterItem, sizeof(SSTRING_FILTER_ITEM));
								RtlUpcaseUnicodeString(pUniStr, pUniStr, FALSE);

								NewFilterItem->Mask = pUniStr;

								KeAcquireSpinLock(&filtersSpinLock, &OldIrql);
								{
									if (ListStart == NULL)
									{
										// �������������� ������ � ����� ������
										ListStart = NewFilterItem;
										ListEnd = NewFilterItem;
									}
									else
									{
										ListEnd->Next = NewFilterItem;
										ListEnd = NewFilterItem;
									}
								}
								KeReleaseSpinLock(&filtersSpinLock, OldIrql);

								ntStatus = STATUS_SUCCESS;
							}
							else
							{
								ntStatus = STATUS_INSUFFICIENT_RESOURCES;
								PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("ReadOnlyness!StringFilter!AddStringFilter memory allocation error for SSTRING_FILTER_ITEM(size=%u)", (int)sizeof(SSTRING_FILTER_ITEM)));
							}
						}

						RtlFreeUnicodeString(&convertedStr);
					}
				}
			}
			else
			{
				ntStatus = STATUS_INSUFFICIENT_RESOURCES;
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("ReadOnlyness!StringFilter!AddStringFilter memory allocation error for unicode string buffer(size=%u)", UniStrBuffSize));
			}
		}
		else
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("ReadOnlyness!StringFilter!AddStringFilter memory allocation error for UNICODE_STRING"));
		}
	}
	finally
	{
		if (DosLink != NULL)
		{
			ExFreePoolWithTag(DosLink->Buffer, MEM_TAG_OBJECT_NAME);
		}

		// ��� ������ ��������� �������
		if (ntStatus != STATUS_SUCCESS)
		{
			if (pUniStr != NULL)
			{
				if (pUniStr->Buffer != NULL)
				{
					ExFreePoolWithTag(pUniStr->Buffer, MEM_TAG_STRFILTER_NAME);
				}

				ExFreePoolWithTag(pUniStr, MEM_TAG_STRFILTER_NAME);
			}
		}
	}

	return ntStatus;
}

/// ������� ���� ��������
void ClearStringFilters()
{
	PSSTRING_FILTER_ITEM item = ListStart;
	KIRQL OldIrql;
	PVOID ItemReleasePointer = NULL;

	while (item != NULL)
	{
		KeAcquireSpinLock(&filtersSpinLock, &OldIrql);
		{
			ListStart = item->Next;
			if (ListStart == NULL)
			{
				ListEnd = NULL;
			}
		}
		KeReleaseSpinLock(&filtersSpinLock, OldIrql);

		// ����������� ������ ��-��� ������ �������
		ExFreePoolWithTag(item->Mask->Buffer, MEM_TAG_STRFILTER_NAME);

		// ����������� ������ ��-��� ��������� ������
		ExFreePoolWithTag(item->Mask, MEM_TAG_STRFILTER_NAME);

		// ��������� ��������� �� ���� ��������� �������� ������ � ��������� �� ����. ������� �� ������������ ������ ���������
		ItemReleasePointer = item;
		item = item->Next;

		// ����������� ������ ���� ��������� ������ ������
		ExFreePoolWithTag(ItemReleasePointer, MEM_TAG_STRFILTER_NAME);
	}
}

/// ��������������� ������� ��������
void DeinitStringFilters()
{
	ClearStringFilters();
}

BOOLEAN MatchInStringFilters(PUNICODE_STRING FileName)
{
	BOOLEAN Result = FALSE;
	if (ListStart == NULL)
	{
		return Result;
	}

	PSSTRING_FILTER_ITEM item = ListStart;
	
	while (item != NULL)
	{
		Result = WildTextCompare(FileName->Buffer, item->Mask->Buffer);
		if (Result == TRUE)
		{
			break;
		}

		item = item->Next;
	}

	return Result;
}

/// �������������� �������� ���������� �� *, ? �� DrBoo ��� ������ � UNICODE 
BOOLEAN WildTextCompare(PWCH pTameText, PWCH pWildText)
{
	PWCH pTameBookmark = (PWCH)0;
	PWCH pWildBookmark = (PWCH)0;

	while (1)
	{
		if (*pWildText == '*')
		{
			while (*(++pWildText) == '*')
			{
			}                          

			if (!*pWildText)
			{
				return TRUE;           
			}

			if (*pWildText != '?')
			{
				while (*pTameText != *pWildText)
				{
					if (!(*(++pTameText)))
						return FALSE;  
				}
			}

			pWildBookmark = pWildText;
			pTameBookmark = pTameText;
		}
		else if (*pTameText != *pWildText && *pWildText != '?')
		{
			if (pWildBookmark)
			{
				if (pWildText != pWildBookmark)
				{
					pWildText = pWildBookmark;

					if (*pTameText != *pWildText)
					{
						pTameText = ++pTameBookmark;
						continue;      
					}
					else
					{
						pWildText++;
					}
				}

				if (*pTameText)
				{
					pTameText++;
					continue;         
				}
			}

			return FALSE;              
		}

		pTameText++;
		pWildText++;

		if (!*pTameText)
		{
			while (*pWildText == '*')
			{
				pWildText++;           
			}

			if (!*pWildText)
			{
				return TRUE;           
			}

			return FALSE;              
		}
	}
}
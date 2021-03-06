/** @file
  BIOS system slot designator information boot time changes.
  SMBIOS type 9.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Limited. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmbiosMisc.h"

#include <Library/SerdesLib.h>

extern SMBIOS_TABLE_TYPE9 MiscSystemSlotDesignationPcie0Data;
extern SMBIOS_TABLE_TYPE9 MiscSystemSlotDesignationPcie1Data;
extern SMBIOS_TABLE_TYPE9 MiscSystemSlotDesignationPcie2Data;
extern SMBIOS_TABLE_TYPE9 MiscSystemSlotDesignationPcie3Data;

VOID
UpdateSlotDesignation (
  IN SMBIOS_TABLE_TYPE9 *InputData
  )
{
    STRING_REF                      TokenToUpdate;
    CHAR16                          *SlotDesignation;
    UINTN                           DesignationStrLen;

    SlotDesignation = AllocateZeroPool ((sizeof (CHAR16)) * SMBIOS_STRING_MAX_LENGTH);
    if (NULL == SlotDesignation)
    {
        return;
    }

    if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie0Data)
    {
        UnicodeSPrint(SlotDesignation, SMBIOS_STRING_MAX_LENGTH - 1, L"PCIE0");
    }
    else if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie1Data)
    {
        UnicodeSPrint(SlotDesignation, SMBIOS_STRING_MAX_LENGTH - 1, L"PCIE1");
    }
    else if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie2Data)
    {
        UnicodeSPrint(SlotDesignation, SMBIOS_STRING_MAX_LENGTH - 1, L"PCIE2");
    }
    else if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie3Data)
    {
        UnicodeSPrint(SlotDesignation, SMBIOS_STRING_MAX_LENGTH - 1, L"PCIE3");
    }

    DesignationStrLen = StrLen (SlotDesignation);

    if (DesignationStrLen > 0 )
    {
        TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_SLOT_DESIGNATION);
        HiiSetString (mHiiHandle, TokenToUpdate, SlotDesignation, NULL);
    }

    FreePool (SlotDesignation);
}

VOID
UpdateSlotUsage(
  IN OUT SMBIOS_TABLE_TYPE9 *InputData
  )
{
    EFI_STATUS        Status;
    serdes_param_t    sSerdesParam;

    Status = OemGetSerdesParam (&sSerdesParam);
    if(EFI_ERROR(Status))
    {
        DEBUG((EFI_D_ERROR, "[%a]:[%dL] OemGetSerdesParam failed %r\n", __FUNCTION__, __LINE__, Status));
        return;
    }

    //
    // PCIE0
    //
    if (((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie0Data) && sSerdesParam.hilink1_mode == EM_HILINK1_PCIE0_8LANE)
    {
        InputData->CurrentUsage = SlotUsageAvailable;
    }

    //
    // PCIE1
    //
    if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie1Data)
    {
        if (sSerdesParam.hilink0_mode == EM_HILINK0_PCIE1_4LANE_PCIE2_4LANE)
        {
            InputData->SlotDataBusWidth = SlotDataBusWidth4X;
        }
    }

    //
    // PCIE2
    //
    if ((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie2Data)
    {
        if (sSerdesParam.hilink0_mode == EM_HILINK0_PCIE1_4LANE_PCIE2_4LANE)
        {
            InputData->SlotDataBusWidth = SlotDataBusWidth4X;
            InputData->CurrentUsage = SlotUsageAvailable;
        }
        else if (sSerdesParam.hilink2_mode == EM_HILINK2_PCIE2_8LANE)
        {
            InputData->CurrentUsage = SlotUsageAvailable;
        }
    }

    //
    // PCIE3
    //
    if (((UINTN)InputData == (UINTN)&MiscSystemSlotDesignationPcie3Data) && sSerdesParam.hilink5_mode == EM_HILINK5_PCIE3_4LANE)
    {
        InputData->CurrentUsage = SlotUsageAvailable;
    }
}

/**
  This function makes boot time changes to the contents of the
  MiscSystemSlotDesignator structure (Type 9).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscSystemSlotDesignation)
{
    CHAR8                              *OptionalStrStart;
    UINTN                              SlotDesignationStrLen;
    EFI_STATUS                         Status;
    EFI_STRING                         SlotDesignation;
    STRING_REF                         TokenToGet;
    SMBIOS_TABLE_TYPE9                 *SmbiosRecord;
    EFI_SMBIOS_HANDLE                  SmbiosHandle;
    SMBIOS_TABLE_TYPE9                 *InputData = NULL;

    //
    // First check for invalid parameters.
    //
    if (RecordData == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    InputData = (SMBIOS_TABLE_TYPE9 *)RecordData;

    UpdateSlotUsage (InputData);

    UpdateSlotDesignation (InputData);

    TokenToGet   = STRING_TOKEN (STR_MISC_SYSTEM_SLOT_DESIGNATION);
    SlotDesignation = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
    SlotDesignationStrLen = StrLen(SlotDesignation);

    //
    // Two zeros following the last string.
    //
    SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE9) + SlotDesignationStrLen + 1 + 1);
    if(NULL == SmbiosRecord)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    (VOID)CopyMem(SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE9));

    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE9);

    OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
    UnicodeStrToAsciiStr(SlotDesignation, OptionalStrStart);
    //
    // Now we have got the full smbios record, call smbios protocol to add this record.
    //

    Status = LogSmbiosData( (UINT8*)SmbiosRecord, &SmbiosHandle);
    if(EFI_ERROR(Status))
    {
      DEBUG((EFI_D_ERROR, "[%a]:[%dL] Smbios Type09 Table Log Failed! %r \n", __FUNCTION__, __LINE__, Status));
    }

    FreePool(SmbiosRecord);

Exit:
    if(SlotDesignation != NULL)
    {
      FreePool(SlotDesignation);
    }

    return Status;
}

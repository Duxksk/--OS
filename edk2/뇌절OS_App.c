#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

EFI_STATUS
ReadFatFile(
    IN  CHAR16 *FilePath,
    OUT VOID  **OutBuffer,
    OUT UINTN  *OutSize
) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_PROTOCOL *Root = NULL;
    EFI_FILE_PROTOCOL *File = NULL;
    EFI_FILE_INFO *Info = NULL;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;
    UINTN InfoSize = 0;
    VOID *Buffer = NULL;

    if (!FilePath || !OutBuffer || !OutSize)
        return EFI_INVALID_PARAMETER;

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    );
    if (EFI_ERROR(Status))
        return Status;

    for (UINTN i = 0; i < HandleCount; i++) {

        Status = gBS->HandleProtocol(
            Handles[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&Fs
        );
        if (EFI_ERROR(Status))
            continue;

        Status = Fs->OpenVolume(Fs, &Root);
        if (EFI_ERROR(Status))
            continue;

        Status = Root->Open(
            Root,
            &File,
            FilePath,
            EFI_FILE_MODE_READ,
            0
        );
        if (EFI_ERROR(Status)) {
            Root->Close(Root);
            continue;
        }

        /* 파일 정보 크기 조회 */
        Status = File->GetInfo(
            File,
            &gEfiFileInfoGuid,
            &InfoSize,
            NULL
        );
        if (Status != EFI_BUFFER_TOO_SMALL)
            goto Cleanup;

        Status = AllocatePool(EfiBootServicesData, InfoSize, (VOID **)&Info);
        if (EFI_ERROR(Status))
            goto Cleanup;

        Status = File->GetInfo(
            File,
            &gEfiFileInfoGuid,
            &InfoSize,
            Info
        );
        if (EFI_ERROR(Status))
            goto Cleanup;

        if (Info->FileSize == 0)
            goto Cleanup;

        if (Info->FileSize > MAX_UINTN)
            goto Cleanup;

        Status = AllocatePool(
            EfiBootServicesData,
            (UINTN)Info->FileSize,
            &Buffer
        );
        if (EFI_ERROR(Status))
            goto Cleanup;

        File->SetPosition(File, 0);
        *OutSize = (UINTN)Info->FileSize;
        Status = File->Read(File, OutSize, Buffer);
        if (EFI_ERROR(Status))
            goto Cleanup;

        *OutBuffer = Buffer;
        Status = EFI_SUCCESS;
        goto Done;

Cleanup:
        if (Buffer) FreePool(Buffer);
        if (Info)   FreePool(Info);
        if (File)   File->Close(File);
        if (Root)   Root->Close(Root);

        Buffer = NULL;
        Info   = NULL;
        File   = NULL;
        Root   = NULL;
    }

    Status = EFI_NOT_FOUND;

Done:
    if (Handles)
        FreePool(Handles);
    return Status;
}

/* ------------------------- */

EFI_STATUS EFIAPI UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
) {
    Print(L"Boot Loading...\r\n");

    VOID *FileBuffer = NULL;
    UINTN FileSize   = 0;

    EFI_STATUS Status = ReadFatFile(
        L"\\EFI\\AsciiArt.txt",
        &FileBuffer,
        &FileSize
    );

    if (EFI_ERROR(Status)) {
        Print(L"File load failed: %r\r\n", Status);
        return Status;
    }

    /* ASCII / UTF-16 자동 판별 */
    CHAR16 *Text16 = NULL;

    if (FileSize >= 2 &&
        ((UINT16 *)FileBuffer)[0] == 0xFEFF) {
        /* UTF-16 (BOM 있음) */
        Text16 = (CHAR16 *)FileBuffer;
    } else {
        /* ASCII → CHAR16 변환 */
        UINTN CharCount = FileSize;
        AllocatePool(
            EfiBootServicesData,
            (CharCount + 1) * sizeof(CHAR16),
            (VOID **)&Text16
        );

        for (UINTN i = 0; i < CharCount; i++)
            Text16[i] = ((CHAR8 *)FileBuffer)[i];

        Text16[CharCount] = L'\0';
        FreePool(FileBuffer);
    }

    Print(L"%s\r\n", Text16);

    /* 이벤트 처리 */
    EFI_EVENT TimerEvent;
    EFI_EVENT Events[2];
    UINTN EventIndex;
    EFI_INPUT_KEY Key;

    gBS->CreateEvent(
        EVT_TIMER,
        TPL_CALLBACK,
        NULL,
        NULL,
        &TimerEvent
    );

    gBS->SetTimer(
        TimerEvent,
        TimerPeriodic,
        10 * 1000 * 1000
    );

    Events[0] = TimerEvent;
    Events[1] = gST->ConIn->WaitForKey;

    while (TRUE) {
        gBS->WaitForEvent(2, Events, &EventIndex);

        if (EventIndex == 1 &&
            !EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &Key))) {

            if (Key.UnicodeChar == L'R' || Key.UnicodeChar == L'r') {
                Print(L"\r\nRebooting...\r\n");
                gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
            }
        }
    }
}

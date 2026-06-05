#pragma warning(push , 0 )
#include <w32api.h>
#include <windows.h>
#include "miniz.h"
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define OPERATION_ADD 1
#define OPERATION_REMOVE 2

int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        printf("Not enough arguments have been supplied!\n");
        printf("It is necessary to inform the operation [+](add file) or [-](remove file) \n");
        printf("It is necessary to inform the file path of the archive to be added or deleted\n");
        printf("It is necessary to inform the file path where the compressed archive will be stored!\n");
        return 0;
    }

    char* Operation = argv[1];
    char* FilePathArchive = argv[2];
    char* FilePathCompression = argv[3];
    int OperationSelected = 0;
    void *FileBuffer;
    HANDLE FileHandle;
    LARGE_INTEGER FileSize;
    DWORD bytesRead;

    Operation = strstr(Operation, "[");
    Operation++;


    if (Operation[0] == '+')
    {
        OperationSelected = OPERATION_ADD;
        printf("The operation selected is add a file to compression!\n");
    }
    else if (Operation[0] == '-')
    {
        OperationSelected = OPERATION_REMOVE;
        printf("The operation selected is to remove a file from compression!\n");
    }
    else
    {
        printf("The operation selected is invalid!\n");
        printf("The operation must either add a file [+] to the compression or remove a file from compression [-] \n");
        goto EXIT;
    }

    if (FilePathArchive == "")
    {
        printf("The path of the file was not informed!\n");

        goto EXIT;
    }

    if (FilePathCompression == "")
    {
        printf("The path of the compressed file was not informed!\n");

        goto EXIT;
    }

    if (GetFileAttributesA(FilePathArchive) == INVALID_FILE_ATTRIBUTES)
    {
        printf("the file specified does not exist!\n");
        goto EXIT;
    }

    if (OperationSelected == OPERATION_ADD)
    {
        FileHandle = CreateFileA(
            FilePathArchive,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

        if (
        FileHandle  == INVALID_HANDLE_VALUE)
        {
            printf("CreateFileA failed with error: %d\n", GetLastError());
            goto EXIT;
        }

        printf("[+] The file %s has been found and was opened for reading!\n", FilePathArchive);



        if (GetFileSizeEx(FileHandle, &FileSize) == 0)
        {
            printf("GetFileSize failed with error: %d\n", GetLastError());
            goto EXIT;
        }

        FileBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileSize.QuadPart);

        if (FileBuffer == NULL) {
            printf("HeapAlloc failed with error: %d\n", ERROR_NOT_ENOUGH_MEMORY);
            goto EXIT;
        }

        if (ReadFile(FileHandle, FileBuffer, (DWORD) FileSize.QuadPart, &bytesRead, NULL) == FALSE)
        {
            printf("ReadFile failed with error: %d\n", GetLastError());
            goto EXIT;
        }

        if (bytesRead != FileSize.QuadPart) {
            printf("the number of bytes read is different than the number of bytes in the file");
            goto EXIT;
        }


        if ((
            mz_zip_add_mem_to_archive_file_in_place
            (
                FilePathCompression,
                FilePathArchive,
                FileBuffer,
                FileSize.QuadPart,
                "",
                1,
                MZ_BEST_COMPRESSION)
                ) == MZ_FALSE)
        {
            DWORD Error;
            Error = ERROR_COMPRESSED_FILE_NOT_SUPPORTED;
            printf("Failed to compress the file %s to archive %s! %d", FilePathArchive, FilePathCompression, Error);
            return Error;
        }

        printf("[+] The file %s has been successfully added to the archive!\n", FilePathArchive);

    }


    EXIT:
    if (FileBuffer) HeapFree(GetProcessHeap(), 0, FileBuffer);
    if (FileHandle && FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);
    return 0;
}

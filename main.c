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
    char* FileToBeCompressed = argv[2];
    char* PathToCompressedFiles = argv[3];
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

    if (FileToBeCompressed == "")
    {
        printf("The path of the file was not informed!\n");

        goto EXIT;
    }

    if (PathToCompressedFiles == "")
    {
        printf("The path of the compressed file was not informed!\n");

        goto EXIT;
    }

    if (GetFileAttributesA(FileToBeCompressed) == INVALID_FILE_ATTRIBUTES)
    {
        printf("the file specified does not exist!\n");
        goto EXIT;
    }

    if (OperationSelected == OPERATION_ADD)
    {
        FileHandle = CreateFileA(
            FileToBeCompressed,
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

        printf("[+] The file %s has been found and was opened for reading!\n", FileToBeCompressed);



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
                PathToCompressedFiles,
                FileToBeCompressed,
                FileBuffer,
                FileSize.QuadPart,
                "",
                1,
                MZ_BEST_COMPRESSION)
                ) == MZ_FALSE)
        {
            DWORD Error;
            Error = ERROR_COMPRESSED_FILE_NOT_SUPPORTED;
            printf("Failed to compress the file %s to archive %s! %d", FileToBeCompressed, PathToCompressedFiles, Error);
            return Error;
        }

        printf("[+] The file %s has been successfully added to the archive %s!\n", FileToBeCompressed, PathToCompressedFiles);

    }

    else if (OperationSelected == OPERATION_REMOVE)
    {
        mz_zip_archive ZipArchive = { 0 };
        int NumberOfFilesInCompressedFile = 0;


        if ((mz_zip_reader_init_file(&ZipArchive, PathToCompressedFiles, 0)) == FALSE)
        {
            printf("Mz_zip_reader_init_file failed with error code %d!\n", mz_zip_get_last_error(&ZipArchive));
            goto EXIT;
        }

        printf("Opened the compressed file for reading!\n");

        NumberOfFilesInCompressedFile = (int)mz_zip_reader_get_num_files(&ZipArchive);

        printf("%d files were found inside the compressed file!\n", NumberOfFilesInCompressedFile);

        for (int CurrentFile = 0; CurrentFile < NumberOfFilesInCompressedFile; CurrentFile++)
        {
            mz_zip_archive_file_stat CompressedFileStatistics = { 0 };

            if (mz_zip_reader_file_stat(&ZipArchive, CurrentFile, &CompressedFileStatistics) == FALSE)
            {

                printf("Mz_zip_reader_file_stat failed with error code %d!\n", mz_zip_get_last_error(&ZipArchive));
                goto EXIT;
            }

            if (_stricmp(CompressedFileStatistics.m_filename, FileToBeCompressed) == 0)
            {
                printf("[-] File %s found in archive %s.\n", FileToBeCompressed, PathToCompressedFiles);

                if ((mz_zip_reader_extract_to_file(&ZipArchive, CurrentFile, FileToBeCompressed, 0)) == FALSE)
                {
                    printf("Mz_zip_reader_extract_to_file failed with error code %d!\n", mz_zip_get_last_error(&ZipArchive));
                    goto EXIT;
                }
                else
                {
                    printf("[-] Successfully extracted file %s.\n", FileToBeCompressed);
                }

                goto EXIT;
            }
        }

        printf("ERROR: File %s not found in archive %s!\n", FileToBeCompressed, PathToCompressedFiles);

    }


    EXIT:
    if (FileBuffer) HeapFree(GetProcessHeap(), 0, FileBuffer);
    if (FileHandle && FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);
    return 0;
}

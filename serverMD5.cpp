/*
===============================================
Title:   server.cpp
Authors: Julian Fliegler, Allison Rose Johnson
Date:    2 May 2022
===============================================
*/

// trying to implement md5 checksum

#define WIN32_LEAN_AND_MEAN     // exclude rarely used Windows headers
#define _WIN32_WINNT 0x0501     // version at least Windows XP

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <limits.h>             // INT_MAX
//#include <process.h>
#include <time.h> // _strtime

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5050
#define DEFAULT_ADDR "127.0.0.1"

// md5
#include <stdio.h>
//#include <windows.h>
#include <wincrypt.h>
//#define BUFSIZE 1024
#define MD5LEN  16
#define PROV_RSA_FULL 1
#define CRYPT_VERIFYCONTEXT 0xF0000000
// #define CALG_MD5 ((4 << 13) | (0) | 3)
// #define HP_HASHVAL 0x0002

// prototypes
int CalcChecksum(char buffer[]);
DWORD WINAPI MyRecv(LPVOID lpParameter);
DWORD getChecksum(char rBuffer[]);

using namespace std;

int main()
{
    /* ref: https://www.youtube.com/watch?v=TP5Q0cs6uNo&t=1228s */
    cout << "\t\t-------- TCP SERVER --------" << endl;
    cout << endl;

    // WSAStartup vars
    WORD wVersionRequested = MAKEWORD(2, 2); // Winsock version 2.2
    WSADATA wsaData;
    int iWsaCleanup;

    // socket vars
    SOCKET TCPServerSocket;
    int iCloseSocket;
    
    // address vars
    struct sockaddr_in TCPServerAdd;
    struct sockaddr_in TCPClientAdd;
    int iTCPClientAdd = sizeof(TCPClientAdd);

    // bind, listen, accept, recv vars
    int iBind;
    int iListen;
    SOCKET sAcceptSocket;
    int iRecv;
    char RecvBuffer[DEFAULT_BUFLEN];
    int iRecvBuffer = sizeof(RecvBuffer);

    // Step 1: WSA Startup
    int err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        cout << "WSAStartup failed with error " << err << endl;
        return 1;
    }
    cout << "WSAStartup successful." << endl;

    // Step 2: Fill socket struct
    TCPServerAdd.sin_family = AF_INET;                      // IPv4 address family
    TCPServerAdd.sin_addr.s_addr = inet_addr(DEFAULT_ADDR); // set server address
    TCPServerAdd.sin_port = htons(DEFAULT_PORT);            // bind to port, convert from host byte order to network byte order

    // Step 3: Create socket
    TCPServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(TCPServerSocket == INVALID_SOCKET){
        cout << "TCP server socket creation failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "TCP server socket creation successful." << endl;

    // Step 4: Bind 
    iBind = bind(TCPServerSocket, (sockaddr*)&TCPServerAdd, sizeof(TCPServerAdd)); // associate local address with socket
    if(iBind == SOCKET_ERROR){
        cout << "Binding failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Binding successful." << endl;

    // Step 5: Listen for incoming connections
    iListen = listen(TCPServerSocket, INT_MAX);
        // backlog = INT_MAX = number of connections to create
    if(iListen == SOCKET_ERROR){
        cout << "Listen failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Listen successful." << endl;

    // Step 6: Accept connection
    // sAcceptSocket = accept(TCPServerSocket, (sockaddr*)&TCPClientAdd, &iTCPClientAdd);
    // if(sAcceptSocket == SOCKET_ERROR){
    //     cout << "Accept failed with error " << WSAGetLastError() << endl;
    //     return 1;
    // }
    // int i = 1;
    // cout << "-- SOCKET" << i <<" connection accepted. --" << endl;
    // cout << endl;

    // /* make acceptor socket non-blocking */
    // u_long iMode = 1; // non-blocking
    // int iResult = ioctlsocket(TCPServerSocket, FIONBIO, &iMode);
    // if (iResult != NO_ERROR)
    // cout << "ioctlsocket failed with error " << iResult << endl;

    // Step 7: Receive data from client
    /* ref: https://www.cplusplus.com/forum/windows/59255/ */
    /* ref: https://stackoverflow.com/questions/15185380/tcp-winsock-accept-multiple-connections-clients */
    int sockNum = 1;
    while (1)
    {
        sAcceptSocket = SOCKET_ERROR;
        while(sAcceptSocket == SOCKET_ERROR)
        {
            sAcceptSocket = accept(TCPServerSocket, (sockaddr*)&TCPClientAdd, &iTCPClientAdd);
        }
        cout << "SOCKET " << sockNum++ << " connected." << endl;
        DWORD dwThreadId;
        CreateThread(NULL, 0, MyRecv, (LPVOID) sAcceptSocket, 0, &dwThreadId);
    }

    // Step 8: Close socket
    // iCloseSocket = closesocket(TCPServerSocket);
    // if(iCloseSocket == SOCKET_ERROR){
    //     cout << "Closing socket failed with error " << WSAGetLastError() << endl;
    //     return 1;
    // }
    // cout << "Socket closed successfully." << endl;

    // Step 9: Cleanup Winsock DLL
    iWsaCleanup = WSACleanup();
    if(iWsaCleanup == SOCKET_ERROR){
        cout << "Cleanup failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Cleanup successful." << endl;

    return 0;
}

DWORD WINAPI MyRecv(LPVOID lpParameter){
    SOCKET acceptSocket = (SOCKET) lpParameter;
    
    // Receive data.
    int iRecv = SOCKET_ERROR;
    char RecvBuffer[1024] = "";
    char TimeBuffer[128];

    while(1)
    {
        _strtime(TimeBuffer);
        ZeroMemory(RecvBuffer, sizeof(RecvBuffer));

        iRecv = recv(acceptSocket, RecvBuffer, sizeof(RecvBuffer), 0);
        cout << TimeBuffer << " Client sent: " << RecvBuffer << endl;
        CalcChecksum(RecvBuffer);
        cout << endl;

        if (iRecv == 0){
            cout << "Connection closed." << endl;
            closesocket(acceptSocket);
            exit(0);
        }    
        else if (iRecv == -1)
        {
            // if(WSAGetLastError() == WSAEMSGSIZE) // client has more data to send 
            // {
            //     cout << "continue" << endl;
            //     continue; // iterate again to get more data
            // } 
            // else{
                // some other error
                cout << "Server receive failed with error " << WSAGetLastError() << endl;
                return 0;
            //}
        }
    }
}
        
int CalcChecksum(char buffer[DEFAULT_BUFLEN]){
    // variables
    int count;
    int Sum = 0;
    // checks if the count is less than te default buffer 
    // adds the total sum
    //  switches the sums's sign 
    for (count = 0; count < DEFAULT_BUFLEN; count++)
        Sum = Sum + buffer[count];
  
    cout << "checksum = " << Sum << endl;
    // returns sum 
    return (Sum );
}

DWORD getChecksum(char rBuffer[])
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[DEFAULT_BUFLEN];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";
    //LPCSTR filename="filename.txt";

    // Logic to check usage goes here.

    // hFile = CreateFile(filename,
    //     GENERIC_READ,
    //     FILE_SHARE_READ,
    //     NULL,
    //     OPEN_EXISTING,
    //     FILE_FLAG_SEQUENTIAL_SCAN,
    //     NULL);

    // if (INVALID_HANDLE_VALUE == hFile)
    // {
    //     dwStatus = GetLastError();
    //     printf("Error opening file %s\nError: %d\n", filename, 
    //         dwStatus); 
    //     return dwStatus;
    // }

    // Get handle to the crypto provider
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        return dwStatus;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return dwStatus;
    }

    while (bResult = ReadFile(hFile, rBuffer, DEFAULT_BUFLEN, &cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return dwStatus;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return dwStatus;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        printf("MD5 hash of file [filename] is: ");
        for (DWORD i = 0; i < cbHash; i++)
        {
            printf("%c%c", rgbDigits[rgbHash[i] >> 4],
                rgbDigits[rgbHash[i] & 0xf]);
        }
        printf("\n");
    }
    else
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\n", dwStatus); 
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return dwStatus; 
}
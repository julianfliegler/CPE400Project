/*
===========================
Title:      client.cpp
Author(s):  Julian Fliegler
Date:       4 May 2022
===========================
*/

#define WIN32_LEAN_AND_MEAN     // exclude rarely used Windows headers
#define _WIN32_WINNT 0x0501     // version at least Windows XP

#define DEFAULT_BUFLEN  1024
#define DEFAULT_PORT    5050
#define DEFAULT_ADDR    "127.0.0.1" // IPv4 loopback address
#define MD5LEN          16

#include <windows.h>    // Winsock
#include <winsock2.h>   // Winsock
#include <ws2tcpip.h>   // Winsock
#include <iostream>
#include <limits.h>     // INT_MAX
#include <time.h>       // _strtime
#include <wincrypt.h>   // MD5

using namespace std;

// globals
bool firstRun = true;   // to record init time

// prototypes
DWORD WINAPI MyRecv(LPVOID lpParameter);
char* GetChecksum(char* data);

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

    // bind, listen, accept vars
    int iBind;
    int iListen;
    SOCKET sAcceptSocket;

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
    TCPServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPv4, TCP
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
    iListen = listen(TCPServerSocket, SOMAXCONN); // length of queue for pending connections set to reasonable value
    if(iListen == SOCKET_ERROR){
        cout << "Listen failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Listen successful." << endl;

    // Step 7: Receive data from client
    /* ref: https://www.cplusplus.com/forum/windows/59255/ */
    /* ref: https://stackoverflow.com/questions/15185380/tcp-winsock-accept-multiple-connections-clients */
    int sockNum = 1; // for printing socket nums
    while (1)
    {
        sAcceptSocket = SOCKET_ERROR;
        while(sAcceptSocket == SOCKET_ERROR)
        {
            sAcceptSocket = accept(TCPServerSocket, (sockaddr*)&TCPClientAdd, &iTCPClientAdd); // accept pending connections
        }
        cout << "SOCKET " << sockNum++ << " connected." << endl;
        
        // create indiviual thread for each socket that executes MyRecv()
        DWORD dwThreadId;
        CreateThread(NULL, 0, MyRecv, (LPVOID) sAcceptSocket, 0, &dwThreadId);
    }
    cout << endl;
}

DWORD WINAPI MyRecv(LPVOID lpParameter){
    SOCKET acceptSocket = (SOCKET) lpParameter; // connected socket
    int iRecv = SOCKET_ERROR;                   // for error checking
    char RecvBuffer[DEFAULT_BUFLEN] = "";
    char TimeBuffer[128];                       // hold time values
    char firstTime[129];                        // hold init time value, for results analysis

    // Receive data
    while(1)
    {
        _strtime(TimeBuffer);                       // copy current local time into buffer
        ZeroMemory(RecvBuffer, sizeof(RecvBuffer)); // empty buffer

        // record initial time
        if(firstRun){
            strncpy(firstTime, TimeBuffer, 128);
            firstRun = false;
        }

        iRecv = recv(acceptSocket, RecvBuffer, sizeof(RecvBuffer), 0);
        if(strlen(RecvBuffer) > 2){ // when data is done sending
            cout << TimeBuffer << " File received." << endl;
            cout << GetChecksum(RecvBuffer); 
        }
        cout << endl;

        // when connection closed, print init time and exit program
        if (iRecv == 0){
            cout << "Connection closed." << endl;
            closesocket(acceptSocket);
            cout << "Init time: " << firstTime << endl;
            exit(0);
        }
        else if (iRecv == -1)
        {
            cout << "Server receive failed with error " << WSAGetLastError() << endl;
            exit(1);
        }
    }
}

char* GetChecksum(char* data)
{
    DWORD dwStatus = 0;                     // for error checking
    DWORD cbHash = 16;                      // size, in bytes, of hash buffer (storing hash value)
    HCRYPTPROV cryptProv;                   // pointer to handle for cryptographic service provider (CSP)
    HCRYPTHASH cryptHash;                   // hash object to store hash
    BYTE hash[16];                          // buffer to store hash value
    char *hex = (char*)"0123456789abcdef";  // for converting from hexadecimal
    char *strHash;                          // char* or "string" of hash value
    
    strHash = (char*)malloc(500);
    memset(strHash, '\0', 500);  // empty buffer

    // get handle to CSP (Microsoft Base Cryptographic Provider v1.0)
    // specifies: general purpose data encryption, non-persistent keys
    if (!CryptAcquireContext(&cryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) // incorporates error checking
    {
        dwStatus = GetLastError();
        cout << "CryptAcquireContext failed: " << dwStatus << endl;
        return NULL;
    }

    // create hash object cryptHash
    // specifies: using handle created above, use MD5 non-keyed algorithm
    if (!CryptCreateHash(cryptProv, CALG_MD5, 0, 0, &cryptHash))
    {
        dwStatus = GetLastError();
        printf("CryptCreateHash failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        return NULL;
    }

    // add data to hash object cryptHash
    if (!CryptHashData(cryptHash, (BYTE*)data, strlen(data), 0))
    {
        dwStatus = GetLastError();
        printf("CryptHashData failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        CryptDestroyHash(cryptHash);
        return NULL;
    }

    // retrieve hash value from cryptHash, store in "hash"
    if (!CryptGetHashParam(cryptHash, HP_HASHVAL, hash, &cbHash, 0))
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        CryptDestroyHash(cryptHash);
        return NULL;
    }

    // store hash value as char*
    for (int i = 0; i < cbHash; i++)
    {
        strHash[i * 2] = hex[hash[i] >> 4];
        strHash[(i * 2) + 1] = hex[hash[i] & 0xF];
    }

    // destroy hash object and release CSP handle
    CryptDestroyHash(cryptHash);
    CryptReleaseContext(cryptProv, 0);

    // return hash value
    return strHash;
}
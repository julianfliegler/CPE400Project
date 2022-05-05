/*
===========================
Title:      client.cpp
Author(s):  Julian Fliegler
Date:       4 May 2022
===========================
*/

#undef UNICODE                  // undefine Unicode (for CreateFile())

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
#include <string>
#include <vector>
#include <cmath>        // cmath::floor() used for isInt()
#include <wincrypt.h>   // MD5

using namespace std;

// globals
struct Client
{
    sockaddr_in TCPServerAdd;           // server address
    SOCKET TCPClientSocket;             // client socket
    string id = "SOCKET";               // used for printing output
    vector<string> fileName;            // files to send over socket
    vector<const char*> cFilePath;      // LPCSTR file path to use with Winsock functions
    char SenderBuffer[DEFAULT_BUFLEN];  // data buffer
    int iSenderBuffer = sizeof(SenderBuffer) + 1;
};
int concurrency = 1;    // default concurrency
vector<Client> client;

// prototypes
HANDLE MyCreateFile(HANDLE fHandle, const char* fPath);
void ReadAndSend(HANDLE fHandle, Client cl, int j);
char* GetChecksum(char* data);
bool IsInt(float k);    // used for validating command-line params

int main(int argc, char **argv)
{
    string filePath;    // hold source folder path

    // Validate command-line params
    if(argc < 2) {      // no folder specified
        cout << "Usage: <exec> <source_folder> <concurrency>" << endl;
        return 1;
    }
    else if(argc < 3){  // no concurrency value specificed
        concurrency = 1;
        filePath = argv[1];
    }
    else if(atoi(argv[2]) < 1 || !IsInt(atof(argv[2]))){
        cout << "Error: Concurrency must be positive integer." << endl;
        return 1;
    }
    else{   // get concurrency and filepath from cmd line
        concurrency = atoi(argv[2]);
        filePath = argv[1]; 
    }

    // Manage command-line params
    string origFilePath = filePath;             // hold original path, no asterisk
    filePath.append("*");                       // add asterisk, need for FindFirstFile()
    const char* cfilePath = filePath.c_str();   // convert to LPCSTR to work with Winsock functions
    client.resize(concurrency);                 // concurrency = num of client sockets

    /* ref: https://www.youtube.com/watch?v=TP5Q0cs6uNo&t=1228s */
    cout << "\t\t-------- TCP CLIENT --------" << endl;
    cout << endl;
   
    // Step 1: WSA Startup
    WORD wVersionRequested = MAKEWORD(2, 2); // Winsock version 2.2
    WSADATA wsaData;
    int iWsaCleanup;

    int err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        cout << "WSAStartup failed with error " << err << endl;
        return 1;
    }
    cout << "WSAStartup successful." << endl;

    // Step 2: Create sockets
    int k = 0; // for client ID values
    for(int i = 0; i < client.size(); i++){
        client[i].id += to_string(++k);                                         // ID clients for result analysis
        client[i].TCPClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // init client socket 
        if(client[i].TCPClientSocket == INVALID_SOCKET){                        // error check
            cout << "TCP client " << client[i].id << " creation failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "TCP client socket creation successful." << endl;

    // Step 3: Fill server socket info
    for(int i = 0; i < client.size(); i++){
        client[i].TCPServerAdd.sin_family = AF_INET;                        // IPv4 address family
        client[i].TCPServerAdd.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);   // set server address
        client[i].TCPServerAdd.sin_port = htons(DEFAULT_PORT);              // bind to port, convert from host-byte-order to network-byte-order
    }

    // Step 4: Connect clients to server socket
    for(int i = 0; i < client.size(); i++){
        int iConnect = connect(client[i].TCPClientSocket, (sockaddr*)&client[i].TCPServerAdd, sizeof(client[i].TCPServerAdd));
        if(iConnect == SOCKET_ERROR){
            cout << "Connection of " << client[i].id << " failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "Connection successful." << endl;

    // Step 5: Get files from folder
    /* ref: https://www.cplusplus.com/forum/general/85870/ */
    HANDLE hFind;                   // search handle to find files
    WIN32_FIND_DATAA FindFileData;  // data struct for use with FindFile functions
    int i = 0, j = 0;               // i clients, each with j files

    hFind = FindFirstFileA(cfilePath, &FindFileData); // find first file at given file path (from command line)
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {  
            i = 0;                  // reset -- clients keep getting files until none left in folder 
            while(i < concurrency)  // for each client socket
            {
                if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) // ignore directories
                {
                    client[i].fileName.push_back(FindFileData.cFileName); // get first file name
                    cout << client[i].id << " has file: " << client[i].fileName[j] << endl;
                    i++; // next file goes to next client socket
                }
                FindNextFileA(hFind, &FindFileData); // find next file

                if(GetLastError() == ERROR_NO_MORE_FILES) // if no more files
                {
                    cout << "No more files in source folder." << endl;
                    cout << endl;
                    break;
                }
            }
            j++; // store files in vector

        } while(GetLastError() != ERROR_NO_MORE_FILES); // until no more files
        FindClose(hFind); // close search handle
    }
    else{
        cout << "ERROR: Invalid file path." << endl;
        return 1;
    }

    // Step 6: Calc checksum, send data to server
    HANDLE hFile;       // file handle
    string fileToSend;  // temp container to perform string concat

    for(int i = 0; i < client.size(); i++) // for i clients
    {
        for(int j = 0; j < client[i].fileName.size(); j++) // each with j files
        {
            fileToSend = "";                       // reset
            fileToSend += origFilePath;            // add file path
            fileToSend += client[i].fileName[j];   // add file name
            client[i].cFilePath.push_back(fileToSend.c_str());   // convert to LPCSTR to use with CreateFile
            hFile = MyCreateFile(hFile, client[i].cFilePath[j]); // open file for reading
            ReadAndSend(hFile, client[i], j); // read files into buffer, send over socket to server
            cout << endl;
        }
        ZeroMemory(client[i].SenderBuffer, sizeof(client[i].SenderBuffer)); // empty buffer
    }
    cout << "Data sent successfully." << endl;

    // Step 7: Close sockets
    for(int i = 0; i < client.size(); i++){
        int iCloseSocket = closesocket(client[i].TCPClientSocket);
        if(iCloseSocket == SOCKET_ERROR){
            cout << "Closing socket failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "Socket closed successfully." << endl;

    // Step 8: Cleanup Winsock DLL
    iWsaCleanup = WSACleanup();
    if(iWsaCleanup == SOCKET_ERROR){
        cout << "Cleanup failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Cleanup successful." << endl;

    return 0;
}

/* ref: https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing */
HANDLE MyCreateFile(HANDLE fHandle, const char* fPath){
    fHandle = CreateFile(
                fPath,                 // file to open
                GENERIC_READ,          // open for reading
                FILE_SHARE_READ,       // share for reading
                NULL,                  // default security
                OPEN_EXISTING,         // existing file only
                FILE_ATTRIBUTE_NORMAL, // normal file
                NULL);                 // no attr. template

    if (fHandle == INVALID_HANDLE_VALUE)
    {
        cout << "CreateFile failed with error " << WSAGetLastError() << endl;
        exit(1);
    }
    return fHandle;
}

void ReadAndSend(HANDLE fHandle, Client cl, int j){
    int iSend = 0;          // for error checking
    DWORD dwBytesRead = 0;  // to know when end of file reached

    // read file contents into SenderBuffer
    if(ReadFile(fHandle, cl.SenderBuffer, cl.iSenderBuffer, &dwBytesRead, NULL) == FALSE)
    {
        cout << "ReadFile failed with error " << WSAGetLastError() << endl;
        CloseHandle(fHandle);
        exit(1);
    }
    // if no error reading data into buffer
    else{
        // send file to server
        cout << "\"" << cl.fileName[j] << "\" sent." << endl;
        iSend = send(cl.TCPClientSocket, cl.SenderBuffer, cl.iSenderBuffer, 0); 
        
        // calculate and print checksum
        cout << GetChecksum(cl.SenderBuffer) << endl;

        if(iSend == SOCKET_ERROR)
        {
            cout << "Client send failed with error " << WSAGetLastError() << endl;
            exit(1);
        }
    }
    // when done reading file, append NULL char
    if (dwBytesRead > 0)
    {
        cl.SenderBuffer[dwBytesRead+1]='\0';
    }
    else
    {
        cout << "No data read from file" << endl;
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

// check if given number is integer
/* ref: https://stackoverflow.com/questions/7646512/testing-if-given-number-is-integer */
bool IsInt(float k)
{
    return floor(k) == k;
}
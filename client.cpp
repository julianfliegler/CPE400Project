// ref: https://www.youtube.com/watch?v=TP5Q0cs6uNo&t=1228s

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <limits.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <MSWSock.h>
#include <cmath>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5050
#define DEFAULT_ADDR "127.0.0.1"

using namespace std;

struct Client
{
	sockaddr_in TCPServerAdd;   // addr of server
	SOCKET TCPClientSocket;		// client socket
    string id = "SOCKET";
    vector<string> fileName;            // associated file
    vector<const char*> cFilePath;    // LPCSTR file path to use with Winsock functions
    char SenderBuffer[DEFAULT_BUFLEN];
    int iSenderBuffer = sizeof(SenderBuffer) + 1;
    int checksum;
};

// prototypes
int CalcChecksum(Client c);
bool isInt(float k);
//size_t numFilesInDirectory(fs::path path);
//void print_md5_sum(unsigned char* md);
void handleFilePath(string fPath, Client *cl, int j);
HANDLE mCreateFile(HANDLE fHandle, const char* fPath);
void mReadAndSend(HANDLE fHandle, Client cl);

int concurrency = 1;
vector<Client> client;

int main(int argc, char **argv)
{
    string filePath;

    // TODO: check if concurrency greater than number of files (out of box?)

    // Validate the parameters
    if(argc < 2) {
        cout << "Usage: <exec> <source_folder> <concurrency>" << endl;
        return 1;
    }
    else if(argc < 3){
        concurrency = 1;
        filePath = argv[1];
    }
    else if(atoi(argv[2]) < 1 || !isInt(atof(argv[2]))){
        cout << "Error: Concurrency must be positive integer." << endl;
        return 1;
    }
    else{
        concurrency = atoi(argv[2]);
        filePath = argv[1]; // TODO: process \'s correctly
    }
    cout << "\t\t-------- TCP CLIENT --------" << endl;
    cout << endl;

    client.resize(concurrency);

    // manage file path
    string origFilePath = filePath;                // hold original path, no asterisk
    filePath.append("*");                       // add asterisk for FindFirstFile
    const char* cfilePath = filePath.c_str();   // convert to LPCSTR to work with C++ ftns

    // local vars
    WORD wVersionRequested = MAKEWORD(2, 2); //MAKEWORD(lowbyte, highbyte)
    WSADATA wsaData;
    int iWsaCleanup;
    int err;
    int iCloseSocket;

    int iConnect;
    //int iSend = 0;

    // Step 1: WSA Startup
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        cout << "WSAStartup failed with error " << err << endl;
        return 1;
    }
    cout << "WSAStartup successful." << endl;

    // Step 2: Create sockets
    for(int i = 0; i < client.size(); i++){
        client[i].id += to_string(i);
        client[i].TCPClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(client[i].TCPClientSocket == INVALID_SOCKET){
            cout << "TCP client " << client[i].id << " creation failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "TCP client socket creation successful." << endl;

    // Step 3: Fill socket struct
    for(int i = 0; i < client.size(); i++){
        client[i].TCPServerAdd.sin_family = AF_INET; // IPv4 address family
        client[i].TCPServerAdd.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);
        client[i].TCPServerAdd.sin_port = htons(DEFAULT_PORT); // bind to port, convert from host byte order to ntk byte order
    }

    // Step 4: Connect to socket
    for(int i = 0; i < client.size(); i++){
        iConnect = connect(client[i].TCPClientSocket, (sockaddr*)&client[i].TCPServerAdd, sizeof(client[i].TCPServerAdd));
        if(iConnect == SOCKET_ERROR){
            cout << "Connection of " << client[i].id << " failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "Connection successful." << endl;

    // Step 5: Get files from folder
    /* ref: https://www.cplusplus.com/forum/general/85870/ */
    string data;
    HANDLE hFind;
    WIN32_FIND_DATAA FindFileData;
    int i = 0, j = 0;

    //int file_descript;

    hFind = FindFirstFileA(cfilePath, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {  
            i = 0;
            
            while(i < concurrency)
            {
                if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
                {
                    client[i].fileName.push_back(FindFileData.cFileName);
                    cout << client[i].id << " has file: " << client[i].fileName[j] << endl;
                    i++;
                }

                FindNextFileA(hFind, &FindFileData);
                if(GetLastError() == ERROR_NO_MORE_FILES)
                {
                    cout << "No more files in source folder." << endl;
                    break;
                }
            }

            j++;

        } while(GetLastError() != ERROR_NO_MORE_FILES); // until no more files
        FindClose(hFind);
    }
    //cout << data;
    //system("PAUSE");

    HANDLE hFile;
    string fileToSend;  // temp container to easily perform str concat

    for(int i = 0; i < client.size(); i++)
    {
        for(int j = 0; j < client[i].fileName.size(); j++)
        {
            //cout << client[i].fileName.size() << endl;

            //handleFilePath(origFilePath, &client[i], j);
            
            fileToSend = "";
            fileToSend += origFilePath;            // add file path
            fileToSend += client[i].fileName[j];   // add file name
            client[i].cFilePath.push_back(fileToSend.c_str());   // convert back to LPCSTR to use with CreateFile

            //cout << client[i].cFilePath[j] << endl;

            hFile = mCreateFile(hFile, client[i].cFilePath[j]);
            mReadAndSend(hFile, client[i]);
        }
        memset(client[i].SenderBuffer, 0, sizeof client[i].SenderBuffer); // empty buffer
    }
    cout << "Data sent successfully." << endl;

    // Step 7: Close socket
    for(int i = 0; i < client.size(); i++){
        iCloseSocket = closesocket(client[i].TCPClientSocket);
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

int CalcChecksum(Client c){
    int checksum = 0;

    // sum of bytes
    for(int i = 0; i < sizeof(c.SenderBuffer); i++){
        checksum += c.SenderBuffer[i];
    }

    // take 2's complement
    // checksum =~ checksum;   // bitwise inversion
    // checksum++;

    cout << "checksum = " << checksum << endl;
    return checksum;
}

// ref: https://stackoverflow.com/questions/7646512/testing-if-given-number-is-integer
bool isInt(float k)
{
  return floor(k) == k;
}

HANDLE mCreateFile(HANDLE fHandle, const char* fPath){
    fHandle = CreateFile(
                fPath, // file to open
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
    else
    {
        //cout << "CreateFile successful." << endl;
    }

    return fHandle;
}

void mReadAndSend(HANDLE fHandle, Client cl){
    int iSend = 0;
    DWORD dwBytesRead = 0;

    if(ReadFile(fHandle, cl.SenderBuffer, cl.iSenderBuffer, &dwBytesRead, NULL) == FALSE)
    {
        cout << "ReadFile failed with error " << WSAGetLastError() << endl;
        CloseHandle(fHandle);
        exit(1);
    }

    else{
        //cout << "ReadFile successful." << endl;
        
        cout << "File sent." << endl;
        CalcChecksum(cl);

        iSend = send(cl.TCPClientSocket, cl.SenderBuffer, cl.iSenderBuffer, 0);
        cout << endl;

        if(iSend == SOCKET_ERROR)
        {
            cout << "Client send failed with error " << WSAGetLastError() << endl;
            exit(1);
        }

    }
 
    if (dwBytesRead > 0)
    {
        // append NULL character
        cl.SenderBuffer[dwBytesRead+1]='\0';
    }
    else
    {
        cout << "No data read from file" << endl;
    }
}
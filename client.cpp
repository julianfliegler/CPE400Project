/*
===============================================
Title:   client.cpp
Authors: Julian Fliegler, Allison Rose Johnson
Date:    2 May 2022
===============================================
*/

#undef UNICODE                  // undefine Unicode (for CreateFile())

#define WIN32_LEAN_AND_MEAN     // exclude rarely used Windows headers
#define _WIN32_WINNT 0x0501     // version at least Windows XP

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5050
#define DEFAULT_ADDR "127.0.0.1"

#include <windows.h>    // Winsock
#include <winsock2.h>   // Winsock
#include <ws2tcpip.h>   // Winsock
#include <iostream>
#include <string>
#include <vector>
#include <cmath>        // cmath::floor() used for isInt()

using namespace std;

// globals
struct Client
{
	sockaddr_in TCPServerAdd;           
	SOCKET TCPClientSocket;		        
    string id = "SOCKET";               // used for printing output
    vector<string> fileName;            // files to send over socket
    vector<const char*> cFilePath;      // LPCSTR file path to use with Winsock functions
    char SenderBuffer[DEFAULT_BUFLEN];  
    int iSenderBuffer = sizeof(SenderBuffer) + 1;   
    int checksum;
};
int concurrency = 1;
vector<Client> client;

// prototypes
HANDLE MyCreateFile(HANDLE fHandle, const char* fPath);
void ReadAndSend(HANDLE fHandle, Client cl);
bool IsInt(float k);    // used for validating command-line params
int CalcChecksum(Client c);

int main(int argc, char **argv)
{
    string filePath;    // hold source folder path

    // Validate command-line params
    if(argc < 2) {
        cout << "Usage: <exec> <source_folder> <concurrency>" << endl;
        return 1;
    }
    else if(argc < 3){
        concurrency = 1;
        filePath = argv[1];
    }
    else if(atoi(argv[2]) < 1 || !IsInt(atof(argv[2]))){
        cout << "Error: Concurrency must be positive integer." << endl;
        return 1;
    }
    else{
        concurrency = atoi(argv[2]);
        filePath = argv[1]; 
    }

    // Manage command-line params
    // file path
    string origFilePath = filePath;             // hold original path, no asterisk
    filePath.append("*");                       // add asterisk, need for FindFirstFile()
    const char* cfilePath = filePath.c_str();   // convert to LPCSTR to work with Winsock functions
    // concurrency
    client.resize(concurrency); // concurrency = num of client sockets

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
        client[i].id += to_string(++k); // ID clients for result analysis
        client[i].TCPClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // init client socket 
        if(client[i].TCPClientSocket == INVALID_SOCKET){ // error check
            cout << "TCP client " << client[i].id << " creation failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    }
    cout << "TCP client socket creation successful." << endl;

    // Step 3: Fill server socket info
    for(int i = 0; i < client.size(); i++){
        client[i].TCPServerAdd.sin_family = AF_INET; // IPv4 address family
        client[i].TCPServerAdd.sin_addr.s_addr = inet_addr(DEFAULT_ADDR); // set server address
        client[i].TCPServerAdd.sin_port = htons(DEFAULT_PORT); // bind to port, convert from host-byte-order to network-byte-order
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

    hFind = FindFirstFileA(cfilePath, &FindFileData); // find first file at given file path (from command-line)
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {  
            i = 0;
            while(i < concurrency) // for each client socket
            {
                if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
                {
                    client[i].fileName.push_back(FindFileData.cFileName); // get first file name
                    cout << client[i].id << " has file: " << client[i].fileName[j] << endl;
                    i++;
                }
                FindNextFileA(hFind, &FindFileData); // find next file

                if(GetLastError() == ERROR_NO_MORE_FILES) // if no more files
                {
                    cout << "No more files in source folder." << endl;
                    break;
                }
            }
            j++;

        } while(GetLastError() != ERROR_NO_MORE_FILES); // until no more files
        FindClose(hFind); // close search handle
    }

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

            // Step 6: Read and send 
            ReadAndSend(hFile, client[i]); // read files into buffer, send over socket to server
        }
        memset(client[i].SenderBuffer, 0, sizeof(client[i].SenderBuffer)); // empty buffer
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

void ReadAndSend(HANDLE fHandle, Client cl){
    int iSend = 0;
    DWORD dwBytesRead = 0;

    if(ReadFile(fHandle, cl.SenderBuffer, cl.iSenderBuffer, &dwBytesRead, NULL) == FALSE)
    {
        cout << "ReadFile failed with error " << WSAGetLastError() << endl;
        CloseHandle(fHandle);
        exit(1);
    }
    else{
        cout << "File sent." << endl;
        CalcChecksum(cl);

        // send file to server
        iSend = send(cl.TCPClientSocket, cl.SenderBuffer, cl.iSenderBuffer, 0); 
        if(iSend == SOCKET_ERROR)
        {
            cout << "Client send failed with error " << WSAGetLastError() << endl;
            exit(1);
        }
        cout << endl;
    }
 
    if (dwBytesRead > 0)
    {
        cl.SenderBuffer[dwBytesRead+1]='\0'; // NULL character
    }
    else
    {
        cout << "No data read from file" << endl;
    }
}

/* ref: https://stackoverflow.com/questions/7646512/testing-if-given-number-is-integer */
bool IsInt(float k)
{
  return floor(k) == k;
}

int CalcChecksum(Client c){
    // variables
    int count;
    int Sum = 0;
    // checks if the count is less than te default buffer 
    // adds the total sum
    for (count = 0; count < sizeof(c.SenderBuffer); count++)
        Sum = Sum + c.SenderBuffer[count];

    cout << "checksum = " << Sum << endl;
    // returns sum
    return (Sum);
}

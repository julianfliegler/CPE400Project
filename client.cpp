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

#define DEFAULT_BUFLEN 8096
#define DEFAULT_PORT 5050
#define DEFAULT_ADDR "127.0.0.1"

using namespace std;

struct Client
{
	sockaddr_in TCPServerAdd;   // addr of server
	SOCKET TCPClientSocket;		// client socket
    string id = "SOCKET";       
    string fileName;            // associated file
    const char* cfileToSend;    // LPCSTR version of file to use with Winsock functions
    char SenderBuffer[DEFAULT_BUFLEN];
    int iSenderBuffer = sizeof(SenderBuffer) + 1;
    int checksum;
};

// prototypes
int CalcChecksum(Client c);

int main(int argc, char **argv)
{   
    int concurrency = 1;
    vector<Client> client;
    string filePath;

    // TODO: check if concurrency greater than number of files (out of box?)

    // Validate the parameters
    if(argc < 2) {
        cout << "Usage: <exec> <source_folder> <concurrency>" << endl;
        return 1;
    }
    else if(argc < 3){
        concurrency = 1;
    }
    else if((int)argv[2] < 1){ // TODO: check if concurrency is int
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

    string nfilePath = filePath;                // hold original path, no asterisk
    filePath.append("*");                       // add asterisk for FindFirstFile
    const char* cfilePath = filePath.c_str();   // convert to LPCSTR to work with C++ ftns

    // local vars
    WORD wVersionRequested = MAKEWORD(2, 2); //MAKEWORD(lowbyte, highbyte)
    WSADATA wsaData;
    int iWsaCleanup;
    int err;
    int iCloseSocket;

    int iConnect;

    /* client doesn't recv anything back */
    // int iRecv;
    // char RecvBuffer[DEFAULT_BUFLEN] = "NULL";
    // int iRecvBuffer = strlen(RecvBuffer); + 1;

    int iSend = 0;

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

    /* client doesn't recv anything back */
    // // Step 5: Receive data from server
    // //for(int i = 0; i < concurrency; i++){
    //     iRecv = recv(client[0].TCPClientSocket, RecvBuffer, iRecvBuffer, 0); // only recv over one socket
    //     if(iRecv == SOCKET_ERROR){
    //         cout << "Client receive failed with error " << WSAGetLastError() << endl;
    //         return 1;
    //     }
    // //}
    // cout << "Data received successfully." << endl;
    // cout << "DATA RECV: ";
    // for(char i; i < strlen(RecvBuffer) + 1; i++){
    //     cout << RecvBuffer[i];
    // } cout << endl;

    // Step 5: Get files from folder
    /* ref: https://www.cplusplus.com/forum/general/85870/ */
    string data;  
    HANDLE hFind;
    WIN32_FIND_DATAA FindFileData; 
    int i = 0;

    hFind = FindFirstFileA(cfilePath, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do
        {
            if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY){
                client[i].fileName = FindFileData.cFileName;
                // data += FindFileData.cFileName;
                // data += '\n'; 
                i++;
            } 
        } while( FindNextFileA( hFind, &FindFileData ) && i < client.size());  
        FindClose( hFind );  
    }
    //cout << data;
    //system("PAUSE"); 
    
    HANDLE hFile;
    string fileToSend;  // temp container to easily perform str concat
    DWORD dwBytesRead = 0;
    
    for(int i=0; i<client.size(); i++)
    {
        fileToSend = "";
        fileToSend += nfilePath;            // add file path
        fileToSend += client[i].fileName;   // add file name
        client[i].cfileToSend = fileToSend.c_str();   // convert back to LPCSTR to use with CreateFile

        dwBytesRead = 0;
        hFile = CreateFile(
                client[i].cfileToSend, // file to open
                GENERIC_READ,          // open for reading
                FILE_SHARE_READ,       // share for reading
                NULL,                  // default security
                OPEN_EXISTING,         // existing file only
                FILE_ATTRIBUTE_NORMAL, // normal file
                NULL);                 // no attr. template

        if (hFile == INVALID_HANDLE_VALUE)
        {
            cout << "CreateFile failed with error " << WSAGetLastError() << endl;
            return 1;
        }
        else
            //cout << "CreateFile successful." << endl;

        if(ReadFile(hFile, client[i].SenderBuffer, client[i].iSenderBuffer, &dwBytesRead, NULL) == FALSE)
        {
            cout << "ReadFile failed with error " << WSAGetLastError() << endl;
            CloseHandle(hFile);
            return 1;
        }
        else
            //cout << "ReadFile successful." << endl;

        if (dwBytesRead > 0)
        {
            // append NULL character
            client[i].SenderBuffer[dwBytesRead+1]='\0';
            //cout << "String read from file has " << dwBytesRead << " bytes" << endl;
        }
        else
        {
            cout << "No data read from file" << endl;
        }
    }  



    // Step 6: Send data to server
    int index = 0;
    do{ 
        // calc checksum
        CalcChecksum(client[index]);
        iSend = send(client[index].TCPClientSocket, client[index].SenderBuffer, client[index].iSenderBuffer, 0);
        //cout << "isend = " << iSend << endl;
        //cout << "DATA SENT: " << client[index].SenderBuffer << endl;
        //system("PAUSE");
        if(iSend == SOCKET_ERROR){
            cout << "Client send failed with error " << WSAGetLastError() << endl;
            return 1;
        }
        index++;
    } while(iSend > 0 && index < client.size());
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
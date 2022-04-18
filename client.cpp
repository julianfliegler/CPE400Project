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
	//bool con;			        //Set true if a client is connected
	sockaddr_in TCPServerAdd;	//Client info like ip address
	SOCKET TCPClientSocket;		//Client socket
	//fd_set set;			    //used to check if there is data in the socket
	//int i;				    //any piece of additional info
    std::ofstream cf;           //file to send
    string id = "SOCKET";
};

int main(int argc, char **argv)
{   
    int concurrency = 1;
    vector<Client> client;
    char* filePath;

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

    // local vars
    WORD wVersionRequested = MAKEWORD(2, 2); //MAKEWORD(lowbyte, highbyte)
    WSADATA wsaData;
    int iWsaCleanup;
    int err;

    int iCloseSocket;

    int iConnect;

    // int iRecv;
    // char RecvBuffer[DEFAULT_BUFLEN] = "NULL";
    // int iRecvBuffer = strlen(RecvBuffer); + 1;

    int iSend;
    char SenderBuffer[DEFAULT_BUFLEN];
    int iSenderBuffer = sizeof(SenderBuffer) + 1;

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

    // get files from folder
    // issue: trasmit file not working
    /* source: https://www.cplusplus.com/forum/general/85870/ */
    static const char* folderPath = "C:\\users\\17752\\desktop\\test\\*";
    string data;  
    HANDLE hFind;
    HANDLE hFile;
    WIN32_FIND_DATAA FindFileData; 
    int i = 0;

    hFind = FindFirstFileA(folderPath, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do
        {
            if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY){
                // if(TransmitFile(client[i].TCPClientSocket, hFind, 0, 0, NULL, NULL, TF_USE_KERNEL_APC & TF_WRITE_BEHIND) == true)
                // {
                //     cout << "TransmitFile successful." << endl;
                // }
                // else
                // {
                //     cout << "TransmitFile failed with error " << WSAGetLastError() << endl;
                //     return 1;
                // }
                data += FindFileData.cFileName;
                data += '\n'; 
                i++;
            }
                
        } while( FindNextFileA( hFind, &FindFileData ) && i < client.size());  

        FindClose( hFind );  
    }
    cout << data;
    //system("PAUSE");    

///// 
    // file transmission testing using hardcoded file
    // issue: not sending all data in file
    DWORD dwBytesRead = 0;
    hFile = CreateFile(
            "text.txt",            // file to open
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
        cout << "CreateFile successful." << endl;


    if(ReadFile(hFile, SenderBuffer, iSenderBuffer, &dwBytesRead, NULL)== FALSE)
    {
        cout << "ReadFile failed with error " << WSAGetLastError() << endl;
        CloseHandle(hFile);
        return 1;
    }
      else
        cout << "ReadFile successful." << endl;

    if (dwBytesRead > 0)
    {
        // NULL character
        SenderBuffer[dwBytesRead+1]='\0';
        cout << "String read from file has " << dwBytesRead << " bytes" << endl;
    }
    else
    {
        cout << "No data read from file" << endl;
    }

    // issue: not sending all data, not sure if client-side error
    // Step 6: Send data to server
    do{ 
        for(int i = 0; i < client.size(); i++){
            iSend = send(client[i].TCPClientSocket, SenderBuffer, iSenderBuffer, 0);
            if(iSend == SOCKET_ERROR){
                cout << "Client send failed with error " << WSAGetLastError() << endl;
                return 1;
            }
        }
    } while(iSend > 0);
    cout << "Data sent successfully." << endl;
    
    cout << "DATA SENT: ";
    for(char i; i < strlen(SenderBuffer) + 1; i++){
        cout << SenderBuffer[i];
    } cout << endl;

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
    
    system("PAUSE");
    return 0;
}

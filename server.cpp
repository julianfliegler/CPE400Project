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

#define DEFAULT_BUFLEN 8096
#define DEFAULT_PORT 5050
#define DEFAULT_ADDR "127.0.0.1"

using namespace std;

int main()
{
    cout << "\t\t-------- TCP SERVER --------" << endl;
    cout << endl;

    // local vars
    WORD wVersionRequested = MAKEWORD(2, 2); //MAKEWORD(lowbyte, highbyte)
    WSADATA wsaData;
    int iWsaCleanup;
    int err;

    SOCKET TCPServerSocket;
    int iCloseSocket;

    /* might need to use sets to be able to use select(), to avoid blocking on accept() */
    // FD_SET* TCPServerSocket;
    // FD_ZERO(&TCPServerSocket);
    // FD_SET(sockfd,&TCPServerSocket);

    /* blocking not working: recv throws error */
    // u_long iMode = 1; // non-blocking
    // int iResult = ioctlsocket(TCPServerSocket, FIONBIO, &iMode);
    // if (iResult != NO_ERROR)
    //     cout << "ioctlsocket failed with error " << iResult << endl;

    struct sockaddr_in TCPServerAdd;
    struct sockaddr_in TCPClientAdd;
    int iTCPClientAdd = sizeof(TCPClientAdd);

    int iBind;
    int iListen;
    SOCKET sAcceptSocket;

    /* server not supposed to send anything back */
    // int iSend;
    // char SenderBuffer[DEFAULT_BUFLEN] = "Hello from Server!";
    // int iSenderBuffer = strlen(SenderBuffer) + 1;
    
    int iRecv = 0;
    char RecvBuffer[DEFAULT_BUFLEN];
    int iRecvBuffer = sizeof(RecvBuffer);// + 1;

    // Step 1: WSA Startup
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        cout << "WSAStartup failed with error " << err << endl;
        return 1;
    }
    cout << "WSAStartup was successful." << endl;

    // Step 2: Fill socket struct
    TCPServerAdd.sin_family = AF_INET; // IPv4 address family
    TCPServerAdd.sin_addr.s_addr = inet_addr(DEFAULT_ADDR); 
    TCPServerAdd.sin_port = htons(DEFAULT_PORT); // bind to port, convert from host byte order to ntk byte order

    // Step 3: Create socket
    TCPServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(TCPServerSocket == INVALID_SOCKET){
        cout << "TCP server socket creation failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "TCP server socket creation successful." << endl;

    // Step 4: Bind 
    // associate local address with socket
    iBind = bind(TCPServerSocket, (sockaddr*)&TCPServerAdd, sizeof(TCPServerAdd));
    if(iBind == SOCKET_ERROR){
        cout << "Binding failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Binding successful." << endl;

    // Step 5: Listen for incoming connections
    iListen = listen(TCPServerSocket, INT_MAX); // TODO: is using INT_MAX efficient
        // backlog = INT_MAX = number of connections to create
    if(iListen == SOCKET_ERROR){
        cout << "Listen failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Listen successful." << endl;

    /* need to accept connection for each socket, done below */
    // Step 6: Accept connection
    // sAcceptSocket = accept(TCPServerSocket, (sockaddr*)&TCPClientAdd, &iTCPClientAdd);
    // if(sAcceptSocket == SOCKET_ERROR){
    //     cout << "Accept failed with error " << WSAGetLastError() << endl;
    //     return 1;
    // }
    // cout << "Connection accepted." << endl;

    /* nonblocking not working, recv throws error */
    // unsigned long iMode = 1; // non-blocking
    // int iResult;
    // iResult = ioctlsocket(sAcceptSocket, FIONBIO, &iMode);
    // if (iResult != NO_ERROR)
    // {
    //     cout << "ioctlsocket failed with error " << WSAGetLastError() << endl;
    //     return 1;
    // }

    /* getting data another way (below), but keeping just in case */
    // Step 8: Receive data from client
    //source: https://www.cplusplus.com/forum/windows/59255/
    // while((iRecv = recv(sAcceptSocket, RecvBuffer, iRecvBuffer, 0)) != 0) // continue till disconnected
    // {
    //     cout << "irecv = " << iRecv << endl;
    //     if(iRecv == -1)
    //     {
    //         if(WSAGetLastError() == WSAEMSGSIZE) // client has more data to send than the buffer can get in one call
    //         {
    //             cout << "continue" << endl;
    //             continue; // iterate again to get more data
    //         } else {
    //             // some other error
    //             cout << "recv failed: ERROR " << WSAGetLastError();
    //             return 1;
    //         }
    //     } else {
    //         // if iResult is numeric then it has retrieved what it needed and no more is currently available so if we call recv() again it will block.
    //         printf("Bytes received: %d\n", iRecv);
    //         cout << RecvBuffer << endl;
    //         memset(RecvBuffer, 0, sizeof RecvBuffer); // empty buffer
    //         break;
    //     }
    // }
    // cout << "Connection closed." << endl;

    /* tried to use select(), need to use fd sets */
    // auto timeout = timeval();
    // timeout.tv_sec = 0;
    // timeout.tv_usec = 100;
    // int retVal = 0;

    // Step 8: Receive data from client
    do{
        /* need to make fd_sets in order to work */
        // retVal = select(0, NULL, NULL, NULL, &timeout);
        // if(retVal == 0) break;
        // cout << "retVal = " << retVal << endl;

        /* accept blocks when no pending connections */
        sAcceptSocket = accept(TCPServerSocket, (sockaddr*)&TCPClientAdd, &iTCPClientAdd);
        cout << "sAccept = " << sAcceptSocket << endl;
        if(sAcceptSocket == SOCKET_ERROR){
            cout << "Accept failed with error " << WSAGetLastError() << endl;
            return 1;
        }
        cout << "Connection accepted." << endl;
    
        iRecv = recv(sAcceptSocket, RecvBuffer, iRecvBuffer, 0);
        if ( iRecv > 0 ){
            cout << "Bytes received: " << iRecv << endl;
            cout << RecvBuffer << endl; // debugging
            memset(RecvBuffer, 0, sizeof RecvBuffer); // empty buffer
        }
        else if ( iRecv == 0 )
            cout << "Connection closed." << endl;
        else{
            cout << "Server receive failed with error " << WSAGetLastError() << endl;
            return 1;
        }
    } while(iRecv > 0 && sAcceptSocket > 0);
    cout << "Data received successfully." << endl;

    // Step 9: Close socket
    iCloseSocket = closesocket(TCPServerSocket);
    if(iCloseSocket == SOCKET_ERROR){
        cout << "Closing socket failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Socket closed successfully." << endl;

    // Step 10: Cleanup Winsock DLL
    iWsaCleanup = WSACleanup();
    if(iWsaCleanup == SOCKET_ERROR){
        cout << "Cleanup failed with error " << WSAGetLastError() << endl;
        return 1;
    }
    cout << "Cleanup successful." << endl;

    // system("PAUSE");
    return 0;
}
#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <locale>
#include <codecvt>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void receiveMessages(SOCKET clientSocket) {
    wchar_t buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived / sizeof(wchar_t)] = L'\0';
            wcout << L"Received from server: " << buffer << endl;
        } else if (bytesReceived == 0) {
            cout << "Connection closed by server." << endl;
            break;
        } else {
            cerr << "Error receiving message. Error code: " << WSAGetLastError() << endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock. Error code: " << WSAGetLastError() << endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket. Error code: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(54000);

    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to connect to server. Error code: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    wchar_t buffer[1024];
    int bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived / sizeof(wchar_t)] = L'\0';
        wcout << L"Server: " << buffer << endl;
    }

    thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();

    wstring message;
    while (true) {
        int targetID;
        wcout << L"Enter the target client ID (or -1 to exit): ";
        wcin >> targetID;
        wcin.ignore();

        if (targetID == -1) break;

        wcout << L"Enter message: ";
        getline(wcin, message);

        wstring fullMessage = to_wstring(targetID) + L" " + message;
        send(clientSocket, (char*)fullMessage.c_str(), fullMessage.size() * sizeof(wchar_t), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

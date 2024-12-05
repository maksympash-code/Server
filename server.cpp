#include <iostream>
#include <winsock2.h>
#include <map>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Глобальні змінні
map<int, SOCKET> clients; // Зберігання клієнтів з їх ID
int clientIDCounter = 1;
ofstream logFile("server.log", ios::app); // Файл для логування
mutex clientMutex;

void logMessage(const string& message) {
    lock_guard<mutex> lock(clientMutex);
    logFile << message << endl;
    cout << message << endl;
}

void broadcastClientList() {
    wstring clientList = L"CLIENTS: ";
    for (const auto& client : clients) {
        clientList += to_wstring(client.first) + L" ";
    }

    for (const auto& client : clients) {
        send(client.second, (char*)clientList.c_str(), clientList.size() * sizeof(wchar_t), 0);
    }
}

void handleClient(int clientID, SOCKET clientSocket) {
    wchar_t buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesReceived / sizeof(wchar_t)] = L'\0';

        wstringstream ss(buffer);
        int targetID;
        wstring message;
        if (!(ss >> targetID) || !getline(ss, message)) {
            wstring errorMessage = L"Invalid message format. Use <TargetID> <Message>.";
            send(clientSocket, (char*)errorMessage.c_str(), errorMessage.size() * sizeof(wchar_t), 0);
            continue;
        }

        if (clients.find(targetID) != clients.end()) {
            wstring fullMessage = L"Message from " + to_wstring(clientID) + L": " + message;
            send(clients[targetID], (char*)fullMessage.c_str(), fullMessage.size() * sizeof(wchar_t), 0);
        } else {
            wstring errorMessage = L"Client ID " + to_wstring(targetID) + L" not found.";
            send(clientSocket, (char*)errorMessage.c_str(), errorMessage.size() * sizeof(wchar_t), 0);
        }
    }

    logMessage("Client " + to_string(clientID) + " disconnected.");
    closesocket(clientSocket);
    {
        lock_guard<mutex> lock(clientMutex);
        clients.erase(clientID);
    }
    broadcastClientList();
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock. Error code: " << WSAGetLastError() << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create server socket. Error code: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(54000);

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Bind failed. Error code: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed. Error code: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    logMessage("Server started. Waiting for clients...");

    while (true) {
        sockaddr_in clientAddress;
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to accept client connection. Error code: " << WSAGetLastError() << endl;
            continue;
        }

        int clientID = clientIDCounter++;
        {
            lock_guard<mutex> lock(clientMutex);
            clients[clientID] = clientSocket;
        }
        logMessage("Client " + to_string(clientID) + " connected.");

        // Надсилаємо клієнту його ID
        wstring welcomeMessage = L"Your Client ID is " + to_wstring(clientID);
        send(clientSocket, (char*)welcomeMessage.c_str(), welcomeMessage.size() * sizeof(wchar_t), 0);

        broadcastClientList();

        thread clientThread(handleClient, clientID, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    logFile.close();
    return 0;
}

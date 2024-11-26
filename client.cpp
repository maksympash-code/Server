#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Функція для отримання повідомлень від сервера
void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Received from server: " << buffer << endl;
        } else if (bytesReceived == 0) {
            cout << "Connection closed by server." << endl;
            break;
        } else {
            cerr << "Error receiving message." << endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock." << endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("172.28.240.1"); // IP сервера
    serverAddress.sin_port = htons(54000); // Порт сервера

    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to connect to server." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // ідентифікатор клієнта від сервера
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, 1024, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Server: " << buffer << endl;
    }

    // Запускаємо потік для постійного отримання повідомлень від сервера
    thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();

    // Цикл для введення та відправлення повідомлень
    string message;
    while (true) {
        int targetID;
        cout << "Enter the target client ID (or -1 to exit): ";
        cin >> targetID;
        cin.ignore();  // Очищаємо буфер після введення ID

        if (targetID == -1) break;

        cout << "Enter message: ";
        getline(cin, message);

        // Форматуємо повідомлення: targetID + message
        string fullMessage = to_string(targetID) + " " + message;
        send(clientSocket, fullMessage.c_str(), fullMessage.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

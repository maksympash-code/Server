#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <locale>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

/*
 * client.cpp
 * Автор: Пащенко Максим
 * Група: Комп'ютерна математика 2
 *
 * Опис:
 * Цей файл реалізує клієнтську частину програми для обміну повідомленнями.
 * Додано логування у файл client.log для відстеження подій.
 */

ofstream logFile("client.log", ios::app);

// Функція для запису повідомлень до лог-файлу
void logMessage(const string& message) {
    logFile << message << endl;
    cout << message << endl; // Одночасно виводимо на екран
}

void receiveMessages(SOCKET clientSocket) {
    wchar_t buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived / sizeof(wchar_t)] = L'\0';
            wstring message(buffer);
            wcout << L"Received from server: " << message << endl;
            logMessage("Received from server: " + string(message.begin(), message.end()));
        } else if (bytesReceived == 0) {
            logMessage("Connection closed by server.");
            break;
        } else {
            logMessage("Error receiving message. Error code: " + to_string(WSAGetLastError()));
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
        logMessage("Failed to connect to server. Error code: " + to_string(WSAGetLastError()));
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    wchar_t buffer[1024];
    int bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived / sizeof(wchar_t)] = L'\0';
        wcout << L"Server says: " << buffer << endl;
        logMessage("Connected to server. Message: " + string(buffer, buffer + wcslen(buffer)));
    }

    thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();

    while (true) {
        wcout << L"Enter target client ID and message: ";
        wstring input;
        getline(wcin, input);

        send(clientSocket, (char*)input.c_str(), input.size() * sizeof(wchar_t), 0);
        logMessage("Sent message: " + string(input.begin(), input.end()));
    }

    closesocket(clientSocket);
    WSACleanup();
    logFile.close();
    return 0;
}

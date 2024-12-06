#include <iostream>
#include <winsock2.h>
#include <map>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>

/*
 * server.cpp
 * Автор: Пащенко Максим
 * Група: Комп'ютерна математика 2
 *
 * Опис:
 * Цей файл реалізує серверну частину програми для обміну повідомленнями між клієнтами.
 * Основні функціональні можливості:
 * 1. Обробка підключення клієнтів.
 * 2. Надсилання повідомлень між клієнтами.
 * 3. Логування подій у файл (server.log).
 * 4. Розсилка списку підключених клієнтів.
 */

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Глобальні змінні
map<int, SOCKET> clients;  // Мапа для збереження клієнтів за їх ID
int clientIDCounter = 1;   // Лічильник для генерації унікальних ID клієнтів
ofstream logFile("server.log", ios::app); // Файл для логування повідомлень
mutex clientMutex;         // М'ютекс для забезпечення потокобезпеки

/**
 * Логування повідомлень в консоль і файл
 * @param message Текст повідомлення, яке потрібно залогувати
 */
void logMessage(const string& message) {
    lock_guard<mutex> lock(clientMutex);
    logFile << message << endl; // Запис у файл
    cout << message << endl;    // Вивід у консоль
}

/**
 * Розсилка списку підключених клієнтів усім клієнтам
 */
void broadcastClientList() {
    wstring clientList = L"CLIENTS: ";
    for (const auto& client : clients) {
        clientList += to_wstring(client.first) + L" "; // Формування списку клієнтів
    }

    for (const auto& client : clients) {
        send(client.second, (char*)clientList.c_str(), clientList.size() * sizeof(wchar_t), 0);
    }
}

/**
 * Обробка клієнтських запитів
 * @param clientID Унікальний ID клієнта
 * @param clientSocket Сокет клієнта
 */
void handleClient(int clientID, SOCKET clientSocket) {
    wchar_t buffer[1024]; // Буфер для прийому даних
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, (char*)buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesReceived / sizeof(wchar_t)] = L'\0'; // Завершення строки

        wstringstream ss(buffer);
        int targetID;
        wstring message;

        // Валідація вхідних даних
        if (!(ss >> targetID) || !getline(ss, message)) {
            wstring errorMessage = L"Invalid message format. Use <TargetID> <Message>.";
            send(clientSocket, (char*)errorMessage.c_str(), errorMessage.size() * sizeof(wchar_t), 0);
            logMessage("Client " + to_string(clientID) + " sent an invalid message format.");
            continue;
        }

        // Надсилання повідомлення до клієнта за ID
        if (clients.find(targetID) != clients.end()) {
            wstring fullMessage = L"Message from " + to_wstring(clientID) + L": " + message;
            send(clients[targetID], (char*)fullMessage.c_str(), fullMessage.size() * sizeof(wchar_t), 0);
        } else {
            wstring errorMessage = L"Client ID " + to_wstring(targetID) + L" not found.";
            send(clientSocket, (char*)errorMessage.c_str(), errorMessage.size() * sizeof(wchar_t), 0);
            logMessage("Client " + to_string(clientID) + " tried to send a message to non-existent client " + to_string(targetID));
        }
    }

    // Закриття з'єднання з клієнтом
    logMessage("Client " + to_string(clientID) + " disconnected.");
    closesocket(clientSocket);
    {
        lock_guard<mutex> lock(clientMutex);
        clients.erase(clientID); // Видалення клієнта з мапи
    }
    broadcastClientList(); // Оновлення списку клієнтів
}

/**
 * Основна функція сервера
 * Запускає сервер, приймає клієнтів і створює для кожного потік обробки.
 */

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
            clients[clientID] = clientSocket; // Додавання клієнта до мапи
        }
        logMessage("Client " + to_string(clientID) + " connected.");

        // Надсилаємо клієнту його ID
        wstring welcomeMessage = L"Your Client ID is " + to_wstring(clientID);
        send(clientSocket, (char*)welcomeMessage.c_str(), welcomeMessage.size() * sizeof(wchar_t), 0);

        broadcastClientList(); // Оновлення списку клієнтів

        thread clientThread(handleClient, clientID, clientSocket); // Створення потоку для обробки клієнта
        clientThread.detach(); // Відокремлення потоку
    }

    closesocket(serverSocket);
    WSACleanup();
    logFile.close();
    return 0;
}
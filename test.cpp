#include <iostream>
#include <thread>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

/**
 * Виконує системну команду для запуску сервера.
 */
void startServer() {
    system("start /B server.exe"); // Запускає server.exe у фоновому режимі
    this_thread::sleep_for(chrono::seconds(2)); // Дає час серверу запуститися
}

/**
 * Тестує клієнта для обміну повідомленнями через сервер.
 * @param clientID ID клієнта
 * @param message Повідомлення для відправлення
 */
void testClient(int clientID, const string& message) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Client " << clientID << ": Failed to create socket. Error: " << WSAGetLastError() << endl;
        return;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(54000);

    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Client " << clientID << ": Failed to connect to server. Error: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        return;
    }

    cout << "Client " << clientID << ": Connected to server." << endl;

    // Формуємо повідомлення у форматі <TargetID> <Message>
    stringstream ss;
    ss << clientID + 1 << " " << message; // Відправляємо іншому клієнту
    string formattedMessage = ss.str();

    // Перевірка: надсилаємо валідне повідомлення
    if (send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0) == SOCKET_ERROR) {
        cerr << "Client " << clientID << ": Failed to send message. Error: " << WSAGetLastError() << endl;
    } else {
        cout << "Client " << clientID << ": Sent message: " << formattedMessage << endl;
    }

    // Перевірка: надсилаємо некоректне повідомлення
    string invalidMessage = "invalid_data";
    if (send(clientSocket, invalidMessage.c_str(), invalidMessage.size(), 0) == SOCKET_ERROR) {
        cout << "Client " << clientID << ": Correctly handled invalid message." << endl;
    } else {
        cerr << "Client " << clientID << ": Error: Invalid message was sent!" << endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    cout << "Starting server for testing..." << endl;
    startServer();

    vector<thread> clientThreads;

    // Створюємо клієнтів і запускаємо їхні тести
    for (int i = 1; i <= 3; ++i) {
        clientThreads.emplace_back(testClient, i, "Hello from client " + to_string(i));
    }

    // Очікуємо завершення тестів клієнтів
    for (auto& t : clientThreads) {
        t.join();
    }

    cout << "Tests completed. Please verify server logs for correct behavior." << endl;

    return 0;
}

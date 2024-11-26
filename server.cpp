#include <iostream>
#include <winsock2.h>
#include <map>
#include <thread>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

map<int, SOCKET> clients;  // Зберігання клієнтів з ID
int clientIDCounter = 1;

void handleClient(int clientID, SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, 1024, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Received from client " << clientID << ": " << buffer << endl;

        stringstream ss(buffer);
        int targetID;
        string message;
        ss >> targetID;
        getline(ss, message);

        // Якщо targetID існує в мапі, пересилаємо повідомлення
        if (clients.find(targetID) != clients.end()) {
            string fullMessage = "Message from " + to_string(clientID) + ": " + message;
            send(clients[targetID], fullMessage.c_str(), fullMessage.size(), 0);
        } else {
            string errorMessage = "Client ID " + to_string(targetID) + " not found.";
            send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        }
    }

    cout << "Client " << clientID << " disconnected." << endl;
    closesocket(clientSocket);
    clients.erase(clientID); // Видаляємо клієнта з мапи
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock." << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create server socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(54000);

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server started. Waiting for clients..." << endl;

    while (true) {
        sockaddr_in clientAddress;
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        int clientID = clientIDCounter++; // Призначаємо унікальний ідентифікатор клієнту
        clients[clientID] = clientSocket;

        // Відправляємо клієнту його ідентифікатор
        string idMessage = "Your client ID is: " + to_string(clientID);
        send(clientSocket, idMessage.c_str(), idMessage.size(), 0);

        cout << "Client " << clientID << " connected." << endl;

        // Запускаємо новий потік для обробки клієнта
        thread clientThread(handleClient, clientID, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
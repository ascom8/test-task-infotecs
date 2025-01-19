#include <iostream>
#include <map>
#include <thread>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

#define PORT 8080

std::multimap<int, char> deserializeMap(const std::string& data) {
    std::multimap<int, char> buffer;
    std::istringstream iss(data);
    std::string pair;
    while (std::getline(iss, pair, ';')) {
        if (!pair.empty()) {
            char value = pair[0];
            int key = std::stoi(pair.substr(2));
            buffer.insert({ key, value });
        }
    }
    return buffer;
}

void outputBuffer(const std::multimap<int, char>& buffer) {
    for (const auto& it : buffer) {
        std::cout << it.second << ": " << it.first << "; ";
    }
    std::cout << "\n";
}

void receiveData(int& client_socket, bool& running) {
    char buffer[1024] = { 0 };

    while (running) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Завершаем строку символом конца строки
            std::cout << "Received from server: " << buffer << std::endl;

            try {
                std::multimap<int, char> receivedMap = deserializeMap(buffer);
                outputBuffer(receivedMap);
            }
            catch (...) {
                std::cerr << "Failed to deserialize data.\n";
            }
        }
        else if (bytes_read == 0 || (bytes_read == -1 && errno == ECONNRESET)) {
            std::cerr << "Server disconnected. Attempting to reconnect...\n";
            running = false;
            break;
        }
        else {
            perror("recv failed");
            running = false;
            break;
        }
    }
}

void connectToServer(int& client_socket, sockaddr_in& server_addr, bool& running) {
    while (!running) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("Socket creation failed");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            std::cout << "Reconnected to server.\n";
            running = true;
            break;
        }
        else {
            perror("Connection to server failed");
            close(client_socket);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

int main() {
    int client_socket;
    sockaddr_in server_addr = {};
    bool running = false;

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        return 1;
    }

    std::thread receiverThread;
    while (true) {
        if (!running) {
            connectToServer(client_socket, server_addr, running);

            if (running) {
                // Создаём поток для приёма данных
                if (receiverThread.joinable()) {
                    receiverThread.join();
                }
                receiverThread = std::thread(receiveData, std::ref(client_socket), std::ref(running));
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    close(client_socket);
    return 0;
}

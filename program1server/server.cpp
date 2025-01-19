#include "class.h"
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <chrono>
#include <atomic>

#define PORT 8080

// Функция обработки подключения клиента
bool handleConnection(int& client_socket, int server_fd) {
    while (true) {
        client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket != -1) {
            std::cout << "Client connected.\n";
            return true;
        }
        else {
            perror("Accept failed");
            std::cout << "Waiting for client to reconnect...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

// Проверка состояния сокета
bool isClientConnected(int client_socket) {
    if (client_socket == -1) {
        return false;
    }

    char buffer;
    ssize_t result = recv(client_socket, &buffer, 1, MSG_PEEK | MSG_DONTWAIT);
    if (result == 0) {
        // Клиент закрыл соединение
        std::cerr << "Client disconnected.\n";
        close(client_socket);
        return false;
    }
    else if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("Error checking client connection");
        return false;
    }

    return true;
}

// Функция отправки строки клиенту
bool sendToClient(int& client_socket, const std::string& buffer) {
    if (client_socket == -1) {
        std::cerr << "Invalid socket. Reconnecting...\n";
        return false;
    }

    size_t totalSent = 0;
    while (totalSent < buffer.size()) {
        ssize_t sentBytes = send(client_socket, buffer.c_str() + totalSent, buffer.size() - totalSent, 0);
        if (sentBytes == -1) {
            if (errno == ECONNRESET || errno == EPIPE) {
                std::cerr << "Client disconnected.\n";
                close(client_socket);
                client_socket = -1;
                return false;
            }
            else {
                perror("Send failed");
                return false;
            }
        }
        totalSent += sentBytes;
    }
    std::cout << "Data sent successfully: " << buffer << std::endl;
    return true;
}

// Функция обработки данных от клиента
void handleClient(int& client_socket, int server_fd) {
    std::mutex buffer_mutex;
    std::condition_variable cv;
    std::atomic<bool> bufferReady(false);

    while (true) {
        std::string inputStr;
        std::cout << "Enter a string: ";
        std::getline(std::cin, inputStr);

        // Проверяем состояние подключения
        if (client_socket == -1 || !isClientConnected(client_socket)) {
            std::cerr << "No client connected, waiting for connection...\n";
            if (!handleConnection(client_socket, server_fd)) {
                std::cerr << "Failed to reconnect to client.\n";
                continue;
            }
        }

        String str(inputStr);
        std::string buffer;

        // Поток обработки строки
        std::thread th1([&str, &buffer, &buffer_mutex, &cv, &bufferReady]() {
            str.processingString(buffer);
            {
                std::lock_guard<std::mutex> lock(buffer_mutex);
                bufferReady = true;
            }
            cv.notify_one();
            });

        // Поток отправки строки клиенту
        std::thread th2([&buffer, &buffer_mutex, &cv, &bufferReady, &client_socket]() {
            std::unique_lock<std::mutex> lock(buffer_mutex);
            cv.wait(lock, [&bufferReady]() { return bufferReady.load(); });

            if (client_socket != -1 && isClientConnected(client_socket)) {
                if (!sendToClient(client_socket, buffer)) {
                    std::cerr << "Failed to send data to client. Skipping...\n";
                }
            }
            else {
                std::cerr << "Client is not connected. Skipping send step.\n";
            }

            bufferReady = false;
            buffer.clear();
            });

        th1.join();
        th2.join();

        if (client_socket == -1 || !isClientConnected(client_socket)) {
            std::cerr << "Client disconnected, waiting for reconnection...\n";
            return;
        }
    }
}

int main() {
    int server_fd, client_socket = -1;
    sockaddr_in server_addr = {};

    // Создание серверного сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 3) == -1) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server is running and waiting for connections on port " << PORT << "...\n";

    while (true) {
        if (client_socket == -1) {
            handleConnection(client_socket, server_fd);
        }

        try {
            handleClient(client_socket, server_fd);
        }
        catch (const std::exception& e) {
            std::cerr << "Error handling client: " << e.what() << "\n";
            close(client_socket);
            client_socket = -1;
        }
    }

    close(client_socket);
    close(server_fd);
    return 0;
}

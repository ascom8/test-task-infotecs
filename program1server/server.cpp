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
#include <stdio.h>
#include <errno.h>

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
            std::cout << "Waiting for client to reconnect...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Задержка перед повторной попыткой
        }
    }
}

// Функция отправки строки клиенту
void sendToClient(int& client_socket, const std::string& buffer) {
    if (client_socket == -1) {
        std::cerr << "Invalid socket. Reconnecting...\n";
        return;  // Если сокет недействителен, прекращаем отправку
    }

    size_t totalSent = 0;
    while (totalSent < buffer.size()) {
        ssize_t sentBytes = send(client_socket, buffer.c_str() + totalSent, buffer.size() - totalSent, 0);
        if (sentBytes == -1) {
            if (errno == ECONNRESET || errno == EPIPE) {
                std::cerr << "Client disconnected. Attempting to reconnect...\n";
                close(client_socket);
                client_socket = -1; // Обнуляем сокет
                return;  // Завершаем передачу, если клиент отключился
            }
            else {
                perror("Send failed");
                return;
            }
        }
        totalSent += sentBytes;
    }
    std::cout << "Data sent successfully: " << buffer << std::endl;
}

// Функция обработки данных от клиента
void handleClient(int& client_socket, int server_fd) {
    std::mutex buffer_mutex;
    std::condition_variable cv;
    bool bufferReady = false;

    while (true) {
        std::string inputStr;
        std::cout << "Enter a string: ";
        std::getline(std::cin, inputStr);

        if (client_socket == -1) {
            std::cerr << "No client connected, waiting for connection...\n";
            if (!handleConnection(client_socket, server_fd)) {
                std::cerr << "Failed to reconnect to client.\n";
                return;
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
        std::thread th2([&str, &buffer, &buffer_mutex, &cv, &bufferReady, &client_socket]() {
            std::unique_lock<std::mutex> lock(buffer_mutex);
            cv.wait(lock, [&bufferReady]() { return bufferReady; });
            sendToClient(client_socket, buffer);

            // Сброс состояния готовности буфера
            bufferReady = false;
            str.clearBuffer(buffer);
        });

        // Ожидание завершения потоков
        th1.join();
        th2.join();
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

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // Переход в режим ожидания входящих подключений
    if (listen(server_fd, 3) == -1) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server is running and waiting for connections on port " << PORT << "...\n";

    while (true) {
        // Ожидаем подключение клиента, если его нет
        if (client_socket == -1 && !handleConnection(client_socket, server_fd)) {
            std::cerr << "Failed to reconnect to client.\n";
            continue;
        }

        handleClient(client_socket, server_fd);  // Работа с клиентом
    }

    close(client_socket);
    close(server_fd);
    return 0;
}

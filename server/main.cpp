#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BACKLOG = 10;
const int BUFFER_SIZE = 4096;

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    std::istringstream request_stream(buffer);
    std::string method, path, version;
    request_stream >> method >> path >> version;

    if (method != "GET") {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_socket, response.c_str(), (int)response.size(), 0);
        closesocket(client_socket);
        return;
    }

    // Remove leading '/' from path
    if (path.length() > 1 && path[0] == '/') {
        path = path.substr(1);
    }
    if (path.empty()) {
        path = "index.html";  // default file
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::string not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, not_found.c_str(), (int)not_found.size(), 0);
        closesocket(client_socket);
        return;
    }

    // Read file content into a string
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string file_content = ss.str();

    // Build HTTP response
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\n";
    response_stream << "Content-Length: " << file_content.size() << "\r\n";
    response_stream << "Content-Type: text/html\r\n";
    response_stream << "Connection: close\r\n";
    response_stream << "\r\n";
    response_stream << file_content;

    std::string response = response_stream.str();
    send(client_socket, response.c_str(), (int)response.size(), 0);

    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    int wsResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsResult != 0) {
        std::cerr << "WSAStartup failed: " << wsResult << std::endl;
        return 1;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Allow reuse of the address
    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, BACKLOG) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    std::vector<std::thread> threads;

    while (true) {
        SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            break;
        }

        // Launch a thread to handle the client
        threads.emplace_back(std::thread(handle_client, client_socket));
        // Optional: detach to avoid growing thread vector indefinitely
        threads.back().detach();
    }

    closesocket(listen_socket);
    WSACleanup();

    return 0;
}

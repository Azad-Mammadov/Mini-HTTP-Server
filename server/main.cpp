// Include Winsock2 for Windows Sockets 2 functions and definitions
#include <winsock2.h> // WSAStartup, socket, bind, listen, accept, send, recv, closesocket, WSAGetLastError, WSACleanup
// Include ws2tcpip for definitions for TCP/IP protocols used with Winsock2
#include <ws2tcpip.h> //  INADDR_ANY

// Include iostream for input and output facilities
#include <iostream>
// Include string for string manipulation support
#include <string>
// Include thread for creating and managing threads
#include <thread>
// Include vector for dynamic array support
#include <vector>
// Include fstream for file stream input and output
#include <fstream>
// Include sstream for string stream input and output
#include <sstream>
// Include direct.h for directory manipulation functions in Windows
#include <direct.h> // _getcwd, 

 // Linker directive to include Winsock2 library for socket functions such as WSAStartup, socket, bind, listen, accept, send, recv, closesocket, WSACleanup, WSAGetLastError
#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BACKLOG = 10; //10 clients can be waiting in the queue to be accepted by the server. 
const int BUFFER_SIZE = 4096; // Buffer size for receiving data

void handle_client(SOCKET client_socket) {
    // Define a buffer to store the received data
    char buffer[BUFFER_SIZE];
    // Receive data from the client socket
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    // If no data is received, close the socket and return
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }
    // Null-terminate the buffer
    buffer[bytes_received] = '\0';

    // Create a string stream from the received data
    std::istringstream request_stream(buffer);
    // Extract the method, path, and version from the request
    std::string method, path, version;
    request_stream >> method >> path >> version;

    // If the method is not GET, send a 405 Method Not Allowed response
    if (method != "GET") {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_socket, response.c_str(), (int)response.size(), 0);
        closesocket(client_socket);
        return;
    }

    // Remove leading '/' from path
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }
    // If the path is empty, set it to index.html
    if (path.empty()) {
        path = "index.html";  // default file
    }

    std::cout << "Opening file: " << path << std::endl;

    // Open the file in binary mode
    std::ifstream file(path, std::ios::binary);
    // If the file cannot be opened, send a 404 Not Found response
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
    char cwd[1024];
    if ( _getcwd(cwd, sizeof(cwd)) != nullptr){
        std::cout << "Current working directory: " << cwd << std::endl;
    }    
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

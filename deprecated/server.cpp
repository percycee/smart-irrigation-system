// PLEASE JUST GIVE UP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <boost.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int ssize_t;
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

using namespace std;
const int moisture_pin = 34;
const int relay_pin = 32;
const int button_pin = 26;
const int led_pin = 27;

// Function to initialize Winsock
/* reference: 
    https://learn.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
*/
#ifdef _WIN32
void init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed!" << endl;
        exit(1);
    }
}

void cleanup_winsock() {
    WSACleanup();
}

void close(int client_socket) {
    closesocket(client_socket);
}
#else
#define init_winsock()
#define cleanup_winsock()
#endif

boost::asio::io_service io;
boost::asio::serial_port* arduino_port = nullptr;

void init_serial(const string& port_name) {
    arduino_port = new boost::asio::serial_port(io, port_name);
    arduino_port->set_option(boost::asio::serial_port_base::baud_rate(9600));
    arduino_port->set_option(boost::asio::serial_port_base::character_size(8));
    arduino_port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    arduino_port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    arduino_port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
}

void cleanup_serial() {
    if (arduino_port) {
        arduino_port->close();
        delete arduino_port;
    }
}

string read_from_arduino() {
    if (arduino_port == nullptr) {
        return "Error: No serial connection.";
    }

    boost::asio::streambuf buf;
    boost::asio::read_until(*arduino_port, buf, "\n"); // Read until newline
    istream input_stream(&buf);
    string response;
    getline(input_stream, response);
    return response;
}

// Send data to Arduino
void send_to_arduino(const string& data) {
    if (arduino_port) {
        boost::asio::write(*arduino_port, boost::asio::buffer(data + "\n"));
    }
}

// read the contents of a file
string read_file(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        return "File not found.";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// write new data to log.txt
void write_to_log(const string& data) {
    ofstream log_file("log.txt", ios_base::app);
    if (!log_file.is_open()) {
        cerr << "Error opening log file for writing." << endl;
        return;
    }
    log_file << data << endl;
}

// Function to handle incoming HTTP requests
void handle_request(int client_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Read the request from the client
    ssize_t length = recv(client_socket, buffer, sizeof(buffer), 0);
    if (length < 0) {
        cerr << "Failed to read request from client." << endl;
        return;
    }

    string request(buffer, length);
    string response_header;
    string response_body;

    if (request.find("GET / HTTP/1.1") != string::npos) {
        // Serve user_interface.html
        response_body = read_file("user_interface.html");
        response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    } else if (request.find("GET /style.css HTTP/1.1") != string::npos) {
        // Serve style.css
        response_body = read_file("style.css");
        response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n";
    } else if (request.find("POST /add HTTP/1.1") != string::npos) {
        // Handle POST request
        string post_data = request.substr(request.find("\r\n\r\n") + 4);
        if (!post_data.empty()) {
            write_to_log(post_data);  // Write new data to log.txt
            response_body = "Data added successfully.";
            response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
        } else {
            response_body = "Invalid data.";
            response_header = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n";
        }
    } else {
        response_header = "HTTP/1.1 404 Not Found\r\n\r\n";
        response_body = "404 Not Found";
    }

    // Send the response
    send(client_socket, response_header.c_str(), response_header.length(), 0);
    send(client_socket, response_body.c_str(), response_body.length(), 0);

    // Close the client connection
    close(client_socket);
}

// Main function to start the server
void start_server() {
    // Initialize Winsock on Windows
    init_winsock();

    // Create a TCP socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Failed to create socket." << endl;
        cleanup_winsock();
        exit(1);
    }

    // Set up the server address structure
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);  // Listen on port 8080

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Failed to bind socket." << endl;
        close(server_socket);
        cleanup_winsock();
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        cerr << "Failed to listen on socket." << endl;
        close(server_socket);
        cleanup_winsock();
        exit(1);
    }

    cout << "Server running on port 8080..." << endl;

    // Accept and handle incoming client connections
    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        // Handle the client request
        handle_request(client_socket);
    }

    // Close the server socket and clean up Winsock
    close(server_socket);
    cleanup_winsock();
}

int main() {
    start_server();
    return 0;
}

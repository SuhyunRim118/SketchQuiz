#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
// #include <opencv2/opencv.hpp>
#include "opencv4/opencv2/opencv.hpp"

#define PORT 8080

using namespace cv;

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to a specific port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept client connections
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    namedWindow("Received Drawing", WINDOW_NORMAL);

    while (1) {
        // Receive the size of the drawing data from the client
        int size;
        valread = recv(new_socket, &size, sizeof(size), 0);

        // Receive the drawing data from the client
        std::vector<uchar> data;
        data.resize(size);
        int bytesReceived = 0;
        while (bytesReceived < size) {
            valread = recv(new_socket, &data[bytesReceived], size - bytesReceived, 0);
            bytesReceived += valread;
        }

        // Decode the received drawing data into an OpenCV Mat object
        Mat drawing = imdecode(data, IMREAD_COLOR);

        // Display the received drawing
        imshow("Received Drawing", drawing);

        // Break the loop if 'q' is pressed
        if (waitKey(1) == 'q')
            break;
    }

    return 0;
}

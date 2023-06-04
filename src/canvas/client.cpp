#include "opencv4/opencv2/opencv.hpp"
// #include <opencv2/opencv.hpp>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

using namespace cv;

Mat temp;
Point p;
bool isDrawing;

void onMouse(int event, int x, int y, int flags, void* param)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        // Start drawing
        isDrawing = true;
        p = cv::Point(x, y);
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        // Stop drawing
        isDrawing = false;
    }
    else if (event == cv::EVENT_MOUSEMOVE && isDrawing)
    {
        temp = *(Mat*)param;

        // Draw a line from the previous point to the current point
        cv::Point currPt(x, y);
        cv::line(temp, p, currPt, cv::Scalar(255, 255, 255), 3);
        p = currPt;
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        // Clear the canvas
        temp = cv::Scalar(0, 0, 0);
    }
}

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[65536] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    namedWindow("Canvas", WINDOW_NORMAL);
    Mat canvas = Mat::zeros(600, 800, CV_8UC3);
    setMouseCallback("Canvas", onMouse, &canvas);

    while (true) {
        // Display the canvas with the drawing
        imshow("Canvas", canvas);

        // Convert the canvas to a byte array
        std::vector<uchar> drawingData;
        imencode(".jpg", canvas, drawingData);

        // Send the size of the drawing data to the server
        int size = drawingData.size();
        send(sock, &size, sizeof(size), 0);

        // Send the drawing data to the server in smaller chunks
        const int chunkSize = 4096;
        int bytesSent = 0;
        while (bytesSent < size) {
            int bytesToSend = std::min(chunkSize, size - bytesSent);
            send(sock, drawingData.data() + bytesSent, bytesToSend, 0);
            bytesSent += bytesToSend;
        }

        // Break the loop if 'q' is pressed
        if (waitKey(1) == 'q')
            break;
    }

    return 0;
}

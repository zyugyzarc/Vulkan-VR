#include "webcam.h"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    Webcam cam;
    cam.startStreaming();
    
    std::cout << "Press ESC to exit..." << std::endl;

    while (true) {
        // std::cout << "[main] wait\n";
        Image& frame = cam.getNextImage();
        // std::cout << "[main] got\n";

        // Convert vk::Image to cv::Mat
        int width = frame.width;  // Set appropriate width
        int height = frame.height; // Set appropriate height
        cv::Mat img(height, width, CV_8UC3);

        void* mappedMemory = frame.map();
        std::memcpy(img.data, mappedMemory, img.total() * img.elemSize());
        frame.unmap(mappedMemory);

        // Show the image
        cv::imshow("Webcam Preview", img);

        // Break the loop on ESC key press
        if (cv::waitKey(1) == 27) {
            break;
        }
    }

    cam.stopStreaming();
    cv::destroyAllWindows();
    return 0;
}

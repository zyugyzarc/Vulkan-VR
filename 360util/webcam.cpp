#include "webcam.h"
#include <iostream>
#include <curl/curl.h>
#include <opencv2/opencv.hpp>

// Thanks to
// https://community.theta360.guide/t/preview-mjpeg-stream-on-a-ricoh-theta-x-with-python-and-opencv/8919/7

using namespace cv;

const std::vector<uint8_t> JPEG_SOI = {0xFF, 0xD8};  // JPEG Start of Image
const std::vector<uint8_t> JPEG_EOI = {0xFF, 0xD9};  // JPEG End of Image

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* webcam = static_cast<Webcam*>(userp);
    size_t totalSize = size * nmemb;
    auto* data = static_cast<uint8_t*>(contents);

    // std::cout << "[work:cam] chunk " << totalSize << " bytes\n";

    std::lock_guard<std::mutex> lock(webcam->bufferMutex);

    // Append incoming data to buffer
    webcam->streamBuffer.insert(webcam->streamBuffer.end(), data, data + totalSize);

    // Search for a full JPEG frame
    while (true) {
        auto itStart = std::search(webcam->streamBuffer.begin(), webcam->streamBuffer.end(), JPEG_SOI.begin(), JPEG_SOI.end());
        auto itEnd = std::search(webcam->streamBuffer.begin(), webcam->streamBuffer.end(), JPEG_EOI.begin(), JPEG_EOI.end());

        if (itStart != webcam->streamBuffer.end() && itEnd != webcam->streamBuffer.end() && itEnd > itStart) {

            // std::cout << "[work:cam] frame\n";

            // Extract JPEG data
            std::vector<uint8_t> jpgData(itStart, itEnd + 2);
            
            // Decode image using OpenCV
            cv::Mat img = cv::imdecode(jpgData, IMREAD_COLOR);

            if (!img.empty()) {
                // Store in ring buffer
                if (!webcam->imageBuffer[webcam->head]) {
                    webcam->imageBuffer[webcam->head] = new Image(img.cols, img.rows);
                }

                void* mappedMemory = webcam->imageBuffer[webcam->head]->map();
                std::memcpy(mappedMemory, img.data, img.total() * img.elemSize());
                webcam->imageBuffer[webcam->head]->unmap(mappedMemory);

                // Update buffer indices
                webcam->head = (webcam->head + 1) % Webcam::BUFFER_SIZE;
                if (webcam->head == webcam->tail) {
                    webcam->isBufferFull = true;
                }

                webcam->bufferCondVar.notify_one();
            }

            // Erase processed data
            webcam->streamBuffer.erase(webcam->streamBuffer.begin(), itEnd + 2);
        } else {
            break;  // Wait for more data if no full frame is found
        }
    }

    return totalSize;
}

Webcam::Webcam()
    : running(false), head(0), tail(0), isBufferFull(false),
      cameraUrl("http://192.168.1.1/osc/commands/execute") {
    imageBuffer.resize(BUFFER_SIZE, nullptr);
    // std::cout << "[main:cam] init\n";
}

Webcam::~Webcam() {
    stopStreaming();
    for (auto& img : imageBuffer) {
        delete img;
    }
    // std::cout << "[main:cam] deinit\n";
}

void Webcam::startStreaming() {
    if (running.load()) return;
    // std::cout << "[main:cam] start\n";
    running.store(true);
    streamingThread = std::thread(&Webcam::fetchFrames, this);
}

void Webcam::stopStreaming() {
    if (!running.load()) return;
    // std::cout << "[main:cam] stop\n";
    running.store(false);
    if (streamingThread.joinable()) {
        streamingThread.join();
    }
}

Image& Webcam::getNextImage() {
    // std::cout << "[main:cam] next\n";
    std::unique_lock<std::mutex> lock(bufferMutex);
    bufferCondVar.wait(lock, [this] { return head != tail || isBufferFull; });
    // std::cout << "[main:cam] next - allow\n";
    Image* nextImage = imageBuffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    isBufferFull = false;
    return *nextImage;
}

void Webcam::fetchFrames() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return;
    }

    // std::cout << "[work:cam] start\n";


    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, cameraUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"name\":\"camera.getLivePreview\"}");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);  // No timeout, continuous stream

    // std::cout << "[work:cam] start work\n";

    curl_easy_perform(curl);  // Blocking call, continuously receives data

    // std::cout << "[work:cam] end work\n";

    curl_easy_cleanup(curl);
}

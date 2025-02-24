#include "webcam.h"
#include <iostream>
#include <curl/curl.h>
#include <vector>
#include <mutex>
#include <thread>
#include <cstring>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "../.util/stb_image.h"  // Include stb_image for JPEG decoding

namespace cm {

const std::vector<uint8_t> JPEG_SOI = {0xFF, 0xD8};  // JPEG Start of Image
const std::vector<uint8_t> JPEG_EOI = {0xFF, 0xD9};  // JPEG End of Image

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* webcam = static_cast<Webcam*>(userp);
    size_t totalSize = size * nmemb;
    auto* data = static_cast<uint8_t*>(contents);

    std::lock_guard<std::mutex> lock(webcam->bufferMutex);

    // Append incoming data to buffer
    webcam->streamBuffer.insert(webcam->streamBuffer.end(), data, data + totalSize);

    // Search for a full JPEG frame
    while (true) {
        auto itStart = std::search(webcam->streamBuffer.begin(), webcam->streamBuffer.end(), JPEG_SOI.begin(), JPEG_SOI.end());
        auto itEnd = std::search(webcam->streamBuffer.begin(), webcam->streamBuffer.end(), JPEG_EOI.begin(), JPEG_EOI.end());

        if (itStart != webcam->streamBuffer.end() && itEnd != webcam->streamBuffer.end() && itEnd > itStart) {
            // Extract JPEG data
            std::vector<uint8_t> jpgData(itStart, itEnd + 2);
            
            // Decode image using stb_image
            int width, height, channels;
            // std::cout << "[STB] image with size " << width << "x" << height << "x" << channels << "\n";
            unsigned char* imgData = stbi_load_from_memory(jpgData.data(), jpgData.size(), &width, &height, &channels, 3);  // Force RGB

            if (imgData) {
                // Store in ring buffer
                webcam->imageBuffer[webcam->head]->mapped( [&](void* mappedMemory) {
                    for (int i = 0, j = 0; i < width * height * 3; i++) {
                        ((unsigned char*) mappedMemory)[j] = imgData[i];
                        if (i%3 == 2) j++;
                        j++;
                    }
                    
                    // std::memcpy(mappedMemory, imgData, width * height * 3);
                });

                // Update buffer indices
                webcam->head = (webcam->head + 1) % Webcam::BUFFER_SIZE;
                if (webcam->head == webcam->tail) {
                    webcam->isBufferFull = true;
                }

                webcam->bufferCondVar.notify_one();

                // Free stb_image buffer
                stbi_image_free(imgData);
            }

            // Erase processed data
            webcam->streamBuffer.erase(webcam->streamBuffer.begin(), itEnd + 2);
        } else {
            break;  // Wait for more data if no full frame is found
        }
    }

    return totalSize;
}

Webcam::Webcam(vk::Device& d)
    : running(false), head(0), tail(0), isBufferFull(false),
      cameraUrl("http://192.168.1.1/osc/commands/execute") {
    
    for (int i = 0; i < BUFFER_SIZE; i++) {

        vk::Image* img = new vk::Image(
            d, {
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .extent = {1024, 512, 1},
                .tiling = VK_IMAGE_TILING_LINEAR,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            },
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        img->view({
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
        });

        imageBuffer.push_back(img);
    }

}

Webcam::~Webcam() {
    stopStreaming();
    for (auto& img : imageBuffer) {
        delete img;
    }
}

void Webcam::startStreaming() {
    if (running.load()) return;
    running.store(true);
    streamingThread = std::thread(&Webcam::fetchFrames, this);
}

void Webcam::stopStreaming() {
    if (!running.load()) return;
    running.store(false);
    if (streamingThread.joinable()) {
        streamingThread.join();
    }
}

vk::Image& Webcam::getNextImage() {
    std::unique_lock<std::mutex> lock(bufferMutex);
    bufferCondVar.wait(lock, [this] { return head != tail || isBufferFull; });

    vk::Image* nextImage = imageBuffer[tail];
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

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, cameraUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"name\":\"camera.getLivePreview\"}");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);  // No timeout, continuous stream

    curl_easy_perform(curl);  // Blocking call, continuously receives data

    curl_easy_cleanup(curl);
}

};
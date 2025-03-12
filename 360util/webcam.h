#ifndef WEBCAM_H
#define WEBCAM_H

// Thanks to
// https://community.theta360.guide/t/preview-mjpeg-stream-on-a-ricoh-theta-x-with-python-and-opencv/8919/7

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

#ifndef HEADER
    #define HEADER
        #include "../vk/device.h"
        #include "../vk/image.h"
    #undef HEADER
#else
    #include "../vk/device.h"
    #include "../vk/image.h"
#endif

namespace cm {

class Webcam {
public:
    Webcam(vk::Device&);
    ~Webcam();

    void startStreaming();
    void stopStreaming();
    vk::Image& getNextImage();

    static constexpr size_t BUFFER_SIZE = 5;  // Ring buffer size

    std::vector<VkImageView> allimageviews();

private:
    void fetchFrames();  // Background thread function

    std::thread streamingThread;
    std::mutex bufferMutex;
    std::condition_variable bufferCondVar;
    std::atomic<bool> running;

    std::vector<vk::Image*> imageBuffer;
    size_t head;
    size_t tail;
    bool isBufferFull;

    std::string cameraUrl;
    std::vector<uint8_t> streamBuffer;  // Buffer for incoming data

    friend size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

};

#endif // WEBCAM_H

#ifndef WEBCAM_H
#define WEBCAM_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

class Image {
    unsigned char data[1024 * 1024 * 32];
public:
    uint32_t width, height;
    Image(uint32_t w, uint32_t h) : width(w), height(h) {};
    void* map() {return data;}
    void unmap(void*) {/*no-op*/}
};

class Webcam {
public:
    Webcam();
    ~Webcam();

    void startStreaming();
    void stopStreaming();
    Image& getNextImage();

    static constexpr size_t BUFFER_SIZE = 5;  // Ring buffer size

private:
    void fetchFrames();  // Background thread function

    std::thread streamingThread;
    std::mutex bufferMutex;
    std::condition_variable bufferCondVar;
    std::atomic<bool> running;

    std::vector<Image*> imageBuffer;
    size_t head;
    size_t tail;
    bool isBufferFull;

    std::string cameraUrl;
    std::vector<uint8_t> streamBuffer;  // Buffer for incoming data

    friend size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // WEBCAM_H

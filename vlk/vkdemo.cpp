#include "vklib.h"

#include <iostream>

int main() {
    
    vk::Instance& instance = *new vk::Instance();
    vk::Device& dev = *new vk::Device(instance);

    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);

    dev.init();

    for (int i = 0; instance.update() && i < 100000000; i++) {
        //
    }

    delete &dev;
    delete &instance;

    return 0;
}
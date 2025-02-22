# Compiler and flags
CXX = g++
CFLAGS = -std=c++23 -Og -g
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Source files and object files
SRCS = vkdemo.cpp vk/instance.cpp vk/device.cpp vk/queue.cpp vk/image.cpp vk/shadermodule.cpp vk/pipeline.cpp vk/commandbuffer.cpp vk/buffer.cpp sc/mesh.cpp sc/material.cpp sc/camera.cpp sc/entity.cpp
OBJS = $(SRCS:.cpp=.o)

# Target output
TARGET = vkdemo

# Default rule
all: $(TARGET) # clean

# Rule for compiling vkdemo.cpp with -DHEADER
vkdemo.o: vkdemo.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< -DHEADER

# Rule for compiling other .cpp files
%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<

# Rule for linking object files to the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Clean up generated files
clean:
	@rm -f $(OBJS)

# new stuff

cvtest:
	g++ 360util/webcam.cpp -c -o 360util/webcam.o `pkg-config --cflags --libs opencv4` -lcurl
	g++ 360util/webcam_test.cpp -c -o 360util/webcam_test.o `pkg-config --cflags --libs opencv4` -lcurl
	g++ -o cmdemo 360util/webcam.o 360util/webcam_test.o `pkg-config --cflags --libs opencv4` -lcurl
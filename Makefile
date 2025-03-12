# Compiler and flags
CXX = g++
CFLAGS = -std=c++23 -Og -g
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lcurl

# Source files and object files
SRCS =  \
	\
	vkdemo.cpp\
	\
	vk/instance.cpp\
	vk/device.cpp\
	vk/queue.cpp\
	vk/image.cpp\
	vk/shadermodule.cpp\
	vk/pipeline.cpp\
	vk/commandbuffer.cpp\
	vk/buffer.cpp\
	vk/renderpass.cpp\
	\
	sc/mesh.cpp\
	sc/material.cpp\
	sc/camera.cpp\
	sc/entity.cpp\
	\
	360util/webcam.cpp

OBJS = $(SRCS:.cpp=.o)

# Target output
TARGET = vkdemo

# Default rule
all: $(TARGET) # clean

# Rule for compiling other .cpp files
%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<

# Rule for linking object files to the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Clean up generated files
clean:
	@rm -f $(OBJS)

CFLAGS="-std=c++17 -Og"
LDFLAGS="-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi"

g++ $CFLAGS -c -o vkdemo.o vkdemo.cpp -DHEADER
g++ $CFLAGS -c -o instance.o instance.cpp
g++ $CFLAGS -c -o device.o device.cpp
g++ $CFLAGS -c -o queue.o queue.cpp
g++ $CFLAGS -c -o image.o image.cpp
g++ $CFLAGS -c -o shadermodule.o shadermodule.cpp
g++ $CFLAGS -c -o pipeline.o pipeline.cpp
g++ $CFLAGS -o vkdemo vkdemo.o instance.o device.o queue.o image.o shadermodule.o pipeline.o $LDFLAGS

rm *.o

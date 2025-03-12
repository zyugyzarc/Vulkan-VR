#include <stdexcept>
#include <string>
// #include <vector>
// #include <vulkan/vk_enum_string_helper.h>

namespace vk {
#define VK_ASSERT(x) { _VkAssert(x, __FILE__, __LINE__); }

inline void _VkAssert (VkResult res, std::string file, int line) {
    if (res != VK_SUCCESS)
        throw std::runtime_error( 
            // std::string(string_VkResult(res))
         + "[[VK ERROR " + std::to_string((int)res) + "]]"
         + " at "
         + file + ":"
         + std::to_string(line)
    );
}
};
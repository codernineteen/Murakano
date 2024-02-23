#pragma once

// vulkan
#include <vulkan/vulkan.h>

// std
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <memory> // for smart pointers
#include <algorithm>
#include <array>
#include <string>

// internal
#include "Types.h"
#include "Conversion.h"

// helper function
namespace util 
{
	static std::vector<char> ReadFile(const std::string& filename) 
	{
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open() || file.bad()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = static_cast<size_t>(file.tellg()); // advangtage of ios::ate - we can use read position to determine size of file
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);     // read all bytes at once

        file.close();
        return buffer;
	}
}

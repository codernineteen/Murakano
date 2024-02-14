#pragma once

// vulkan
#include <vulkan/vulkan.h>

// glfw
#include <GLFW/glfw3.h>

// glm
#include <glm/glm.hpp>

// std
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <memory> // for smart pointers
#include <algorithm>
#include <array>

// helper function
namespace util 
{
	static std::vector<char> ReadFile(const std::string& filename) 
	{
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg(); // advangtage of ios::ate - we can use read position to determine size of file
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);     // read all bytes at once
        file.close();

        return buffer;
	}
}

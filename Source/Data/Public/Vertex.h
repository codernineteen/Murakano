#pragma once

#include "Utilities.h"
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    // getter for vertex bindging description
    static VkVertexInputBindingDescription  GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};       // bindingDescription struct
        bindingDescription.binding = 0;                             // index of the binding in the array of bindings (only one binding exists now.)
        bindingDescription.stride = sizeof(Vertex);				    // number of bytes from one entry to the next  
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // specify how to move between data after each vertex

        return bindingDescription;
    }

    // getter for vertex attribute description
    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{}; // attribute description struct

        /**
        * VkVertexInputAttributeDescription specification
        * 1. binding : index of the binding in the array of bindings
        * 2. location : location directive of the input in the vertex shader.
        * 3. format : implicitly defines the byte size of the attribute data.
        * 4. offset : 'offsetof macro' to get offset of the member from the start of the struct
        */
        // position attribute
        attributeDescriptions[0].binding = 0;                         
        attributeDescriptions[0].location = 0;                        
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; 
        attributeDescriptions[0].offset = offsetof(Vertex, pos);      

        // color attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // texture coordinate attribute
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color;
    }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},


    {{-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    7, 4, 0, 0, 3, 7,
    1, 5, 6, 6, 2, 1,
    5, 1, 0, 0, 4, 5,
    3, 2, 6, 6, 7, 3
};

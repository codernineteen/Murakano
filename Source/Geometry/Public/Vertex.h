#pragma once

#include "Utilities.h"
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

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
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{}; // attribute description struct

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

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color;
    }
};
#pragma once

#include "Utilities.h";
struct Vertex
{
    dx::XMFLOAT3 pos;
    dx::XMFLOAT3 color;
    dx::XMFLOAT2 texCoord;

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
        return  pos.x == other.pos.x && pos.y == other.pos.y && pos.z == other.pos.z &&
            color.x == other.color.x && color.y == other.color.y && color.z == other.color.z &&
            texCoord.x == other.texCoord.x && texCoord.y == other.texCoord.y;
    }
};


struct VertexHash
{
    std::size_t operator()(const Vertex& k) const {
        using std::size_t;
        using std::hash;

        // this hashing algorithm is based on cppreference example - https://en.cppreference.com/w/cpp/utility/hash
        return ((hash<float>()(k.pos.x)
            ^ (hash<float>()(k.pos.y) << 1)) >> 1)
            ^ (hash<float>()(k.pos.z) << 1)
            ^ (hash<float>()(k.color.x) << 1)
            ^ (hash<float>()(k.color.y) << 1)
            ^ (hash<float>()(k.color.z) << 1)
            ^ (hash<float>()(k.texCoord.x) << 1)
            ^ (hash<float>()(k.texCoord.y) << 1);
    }
};
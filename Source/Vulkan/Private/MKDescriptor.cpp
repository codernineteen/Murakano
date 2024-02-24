#include "MKDescriptor.h"

MKDescriptor::MKDescriptor(MKDevice& mkDeviceRef, const VkExtent2D& swapchainExtent)
    : _mkDeviceRef(mkDeviceRef), _vkSwapchainExtent(swapchainExtent)
{
    /**
    * Uniform buffer Object layout binding
    * : every binding needs to be described through VkDescriptorSetLayoutBinding
    */
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;                                         // layout(binding = 0)
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  
	uboLayoutBinding.descriptorCount = 1;                                 // model, view, projection transformation is a single uniform buffer object.
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

    // combine layout binding into single descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

    // create descriptor set layout
    if (vkCreateDescriptorSetLayout(_mkDeviceRef.GetDevice(), &layoutInfo, nullptr, &_vkDescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
	
    // create uniform buffer object
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        util::CreateBuffer(
            _mkDeviceRef.GetPhysicalDevice(), 
            _mkDeviceRef.GetDevice(), bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            _vkUniformBuffers[i], 
            _vkUniformBuffersMemory[i]
        );

        // persistent mapping without unmapping memory.
        vkMapMemory(_mkDeviceRef.GetDevice(), _vkUniformBuffersMemory[i], 0, bufferSize, 0, &_vkUniformBuffersMapped[i]);
    }

    /**
    * Create Descriptor Pool
    * 1. specify descriptor pool size
    * 2. specify descriptor pool creation info
    */
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                                         // descriptor type for uniform buffer object
    poolSize.descriptorCount = SafeStaticCast<uint32, uint32_t>(MAX_FRAMES_IN_FLIGHT);         // descriptor count for each frame in flight

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = SafeStaticCast<uint32, uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if(vkCreateDescriptorPool(_mkDeviceRef.GetDevice(), &poolInfo, nullptr, &_vkDescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");

    // allocate descriptor set
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _vkDescriptorSetLayout);  // same layout for each frame in flight
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _vkDescriptorPool;
    allocInfo.descriptorSetCount = SafeStaticCast<int, uint32>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    // create descriptor sets for each frame in flight
    if(vkAllocateDescriptorSets(_mkDeviceRef.GetDevice(), &allocInfo, _vkDescriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor set!");
    
    // populate descriptor set with actual buffer
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = _vkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _vkDescriptorSets[i];
		descriptorWrite.dstBinding = 0;             // binding = 0
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;       // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        // update descriptor set with specified VkWriteDescriptorSet
		vkUpdateDescriptorSets(_mkDeviceRef.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	}
}

MKDescriptor::~MKDescriptor()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
		vkDestroyBuffer(_mkDeviceRef.GetDevice(), _vkUniformBuffers[i], nullptr);
		vkFreeMemory(_mkDeviceRef.GetDevice(), _vkUniformBuffersMemory[i], nullptr);
	}

    // destroy descriptor pool
    vkDestroyDescriptorPool(_mkDeviceRef.GetDevice(), _vkDescriptorPool, nullptr);

    // destroy descriptor set layout
    vkDestroyDescriptorSetLayout(_mkDeviceRef.GetDevice(), _vkDescriptorSetLayout, nullptr);
}

void MKDescriptor::UpdateUniformBuffer(uint32 currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};

    // Apply model transformation
    auto modelMat = dx::XMMatrixRotationAxis(dx::XMVECTOR{ 0.0f, 0.0f, 1.0f }, time * dx::XMConvertToRadians(90.0f));
    modelMat = dx::XMMatrixTranspose(modelMat);

    // Transform into view space
    auto viewMat = dx::XMMatrixLookAtLH(dx::XMVECTOR{ 2.0f, 2.0f, 2.0f }, dx::XMVECTOR{ 0.0f, 0.0f, 0.0f }, dx::XMVECTOR{ 0.0f, 0.0f, -1.0f });
    viewMat = dx::XMMatrixTranspose(viewMat);
    /**
    * Perspective projection
    * 1. fovy : 45 degree field of view
    * 2. aspect ratio : swapchain extent width / swapchain extent height
    * 3. near plane : 0.1f
    * 4. far plane : 10.0f
    */
    auto projectionMat = dx::XMMatrixPerspectiveFovLH(
        dx::XMConvertToRadians(45.0f), 
        _vkSwapchainExtent.width / SafeStaticCast<uint32, float>(_vkSwapchainExtent.height), 
        0.1f, 
        10.0f
    );
    projectionMat = dx::XMMatrixTranspose(projectionMat);

    // Because SIMD operation is supported, i did multiplication in application side, not in shader side.
    ubo.mvpMat = projectionMat * viewMat * modelMat;

    memcpy(_vkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}
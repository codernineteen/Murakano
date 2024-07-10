#include "DescriptorManager.h"

/**
* ---------- public ----------
*/

MKDescriptorManager::MKDescriptorManager()
{
}

MKDescriptorManager::~MKDescriptorManager()
{
    // destroy descriptor pools
    for(auto& pool : _vkDescriptorPoolReady)
        vkDestroyDescriptorPool(_mkDevicePtr->GetDevice(), pool, nullptr);

    for (auto& pool : _vkDescriptorPoolFull)
        vkDestroyDescriptorPool(_mkDevicePtr->GetDevice(), pool, nullptr);

#ifndef NDEBUG
    MK_LOG("combined image sampler destroyed");
    MK_LOG("texture image view destroyed");
    MK_LOG("texture image destroyed and freed its memory");
    MK_LOG("uniform buffer objects destroyed");
    MK_LOG("descriptor pool destroyed");
    MK_LOG("descriptor set layout destroyed");
#endif
}

void MKDescriptorManager::InitDescriptorManager(MKDevice* mkDevicePtr)
{
    // initialize device pointer and swapchain extent
	_mkDevicePtr = mkDevicePtr;
}


void MKDescriptorManager::AllocateDescriptorSet(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetLayout layout)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
    VkDescriptorPool poolInUse = GetDescriptorPool();

    // specify a single descriptor set allocation info
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolInUse;
    allocInfo.descriptorSetCount = static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.pNext = nullptr;
    
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VkResult res = vkAllocateDescriptorSets(_mkDevicePtr->GetDevice(), &allocInfo, descriptorSets.data());
    
    if (res != VK_SUCCESS)
    {
        _vkDescriptorPoolFull.push_back(poolInUse); // push back the full descriptor pool

        poolInUse = GetDescriptorPool(); // get new descriptor pool
        allocInfo.descriptorPool = poolInUse; // assign new descriptor pool to allocation info

        MK_CHECK(vkAllocateDescriptorSets(_mkDevicePtr->GetDevice(), &allocInfo, descriptorSets.data()));
    }

    _vkDescriptorPoolReady.push_back(poolInUse); // push back the in-use descriptor pool
}

VkDescriptorPool MKDescriptorManager::GetDescriptorPool()
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;

    if (!_vkDescriptorPoolReady.empty())
    {
        newPool = _vkDescriptorPoolReady.back();
        _vkDescriptorPoolReady.pop_back();
    }
    else
    {
        newPool = CreateDescriptorPool(_setsPerPool); // create new descriptor pool and assign it to in-use pool

        if (_setsPerPool < 1024)
            _setsPerPool = _setsPerPool * 1.5; // gradually increase the number of sets per pool
        else
            _setsPerPool = 1024; // if it exceeds 1024, fix it to 1024
    }

    return newPool;
}

VkDescriptorPool MKDescriptorManager::CreateDescriptorPool(uint32 setCount)
{
    for (auto& poolSize : _vkDescriptorPoolSizes)
        poolSize.descriptorCount = poolSize.descriptorCount * setCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = SafeStaticCast<size_t, uint32>(_vkDescriptorPoolSizes.size());
    poolInfo.pPoolSizes = _vkDescriptorPoolSizes.data();

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    MK_CHECK(vkCreateDescriptorPool(_mkDevicePtr->GetDevice(), &poolInfo, nullptr, &descriptorPool));

    return descriptorPool;
}

void MKDescriptorManager::ResetDescriptorPool()
{
    for(auto pool : _vkDescriptorPoolReady)
        vkResetDescriptorPool(_mkDevicePtr->GetDevice(), pool, 0);

    for (auto pool : _vkDescriptorPoolReady)
    {
        vkResetDescriptorPool(_mkDevicePtr->GetDevice(), pool, 0);
        _vkDescriptorPoolReady.push_back(pool);
    }

    _vkDescriptorPoolFull.clear();
}

void MKDescriptorManager::AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t binding, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;                 // binding index in shader.
    newBinding.descriptorType = descriptorType;   // type of descriptor
    newBinding.descriptorCount = descriptorCount; // number of descriptors
    newBinding.stageFlags = shaderStageFlags;     // shader stage flags
    newBinding.pImmutableSamplers = nullptr;

    _vkWaitingBindings.push_back(newBinding);
}

void MKDescriptorManager::CreateDescriptorSetLayout(VkDescriptorSetLayout& layout)
{
    // specify descriptor set layout creation info
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32>(_vkWaitingBindings.size());
    layoutInfo.pBindings = _vkWaitingBindings.data();

    // create descriptor set layout
    MK_CHECK(vkCreateDescriptorSetLayout(_mkDevicePtr->GetDevice(), &layoutInfo, nullptr, &layout));

    // be sure to clear layout bindings after creating descriptor set layout !!
    _vkWaitingBindings.clear();
}

void MKDescriptorManager::WriteBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 dstBinding, VkDescriptorType descriptorType)
{
    // To keep memory of buffer info, store it in the deque temporarily
    std::shared_ptr<VkDescriptorBufferInfo> bufferInfo = std::make_shared<VkDescriptorBufferInfo>();
    bufferInfo->buffer = buffer;
    bufferInfo->offset = offset;
    bufferInfo->range = range;
    _vkWaitingBufferInfos.insert(bufferInfo);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = VK_NULL_HANDLE; // specify dst set when update descriptor set
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = descriptorType;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = bufferInfo.get();

	_vkWaitingWrites.push_back(descriptorWrite);
}

void MKDescriptorManager::WriteImageToDescriptorSet(VkImageView imageView, VkSampler imageSampler, VkImageLayout imageLayout, uint32 dstBinding, VkDescriptorType descriptorType)
{
    // To keep memory of image info, store it in the deque temporarily
    std::shared_ptr<VkDescriptorImageInfo> imageInfo = std::make_shared<VkDescriptorImageInfo>();
    imageInfo->imageView = imageView;
    imageInfo->sampler = imageSampler;
    imageInfo->imageLayout = imageLayout;
    _vkWaitingImageInfos.insert(imageInfo);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = VK_NULL_HANDLE; // specify dst set when update descriptor set
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = descriptorType;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = imageInfo.get();

	_vkWaitingWrites.push_back(descriptorWrite);
}

void MKDescriptorManager::WriteAccelerationStructureToDescriptorSet(const VkAccelerationStructureKHR* as, uint32 dstBinding) 
{
    std::shared_ptr<VkWriteDescriptorSetAccelerationStructureKHR> descriptorAsInfo = std::make_shared<VkWriteDescriptorSetAccelerationStructureKHR>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR);
    descriptorAsInfo->accelerationStructureCount = 1;
    descriptorAsInfo->pAccelerationStructures = as;
    descriptorAsInfo->pNext = nullptr;
    _vkWaitingAsWrites.insert(descriptorAsInfo);

    VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptorWrite.dstSet = VK_NULL_HANDLE; // specify dst set when update descriptor set
    descriptorWrite.dstBinding = dstBinding;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pNext = descriptorAsInfo.get();

    _vkWaitingWrites.push_back(descriptorWrite);
}

void MKDescriptorManager::UpdateDescriptorSet(VkDescriptorSet descriptorSet)
{
    for(auto& write : _vkWaitingWrites)
		write.dstSet = descriptorSet;

    // update descriptor sets with waiting writes.
    vkUpdateDescriptorSets(
		_mkDevicePtr->GetDevice(),
		static_cast<uint32>(_vkWaitingWrites.size()),
		_vkWaitingWrites.data(),
		0,
		nullptr
	);

    // clear waiting writes and related buffer/image infos after updating descriptor set
    _vkWaitingBufferInfos.clear();
    _vkWaitingImageInfos.clear();
	_vkWaitingWrites.clear();
}
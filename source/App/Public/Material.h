#pragma once

#include "Pipeline.h"
#include "Allocator.h"

/**
* --------- Enums ----------
*/

enum EMaterialPass
{
	OPAQUE,
	TRANSPARENT,
	UNDEFINED
};

struct MaterialInstance
{
	VkPipeline*      pPipeline;
	uint32           setCount;
	VkDescriptorSet* materialDescriptorSet;
	EMaterialPass    passType;
};

struct GLTFMaterial
{
	MaterialInstance data;
};

struct GeometricSurface
{
	uint32 startIndex;
	uint32 count;
	std::shared_ptr<GLTFMaterial> material;
};


/**
* --------- class definition ----------
*/

class GLTFMetallicRoughnessPipeline
{
public:
	struct MaterialConstants
	{
		alignas(16) XMVECTOR color;
		alignas(16) XMVECTOR metallicRoughness;
	};

	struct MaterialResources
	{
		/* metallic roughness texture */
		std::vector<VkImageView>      textureViews;

		/* separate texture sampler */
		VkSampler textureSampler;

		/* data buffer */
		VkBufferAllocated dataBuffer;
		uint32            dataBufferOffset;
	};

public:
	GLTFMetallicRoughnessPipeline(MKDevice& device);
	~GLTFMetallicRoughnessPipeline();

	/* getter */
	inline VkPipeline GetPipeline(EMaterialPass matPass) const 
	{ 
		return (matPass == EMaterialPass::TRANSPARENT) ? _transparentPipeline.GetPipeline() : _opaquePipeline.GetPipeline(); 
	}
	inline VkPipelineLayout GetPipelineLayout(EMaterialPass matPass) const 
	{ 
		return (matPass == EMaterialPass::TRANSPARENT) ? _transparentPipeline.GetPipelineLayout() : _opaquePipeline.GetPipelineLayout(); 
	}
	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return _materialDescriptorSetLayout; }
	inline VkDescriptorSet GetDescriptorSet(uint32 index) const { return _materialDescriptorSets[index]; }

	MaterialInstance WriteMaterial(EMaterialPass matPass, const MaterialResources& resources);
	void CreateDescriptorSet(uint32 textureSize);
	void BuildPipeline(
		const std::vector<VkPushConstantRange> pcRanges,
		VkDescriptorSetLayout& globalDescLayout,
		std::vector<VkFormat> colorAttachmentFormat,
		VkFormat depthAttachmentFormat
	);

private:
	MKDevice& _deviceRef;

	/* pipelines */
	MKPipeline _opaquePipeline;
	MKPipeline _transparentPipeline;

	/* descriptors */
	VkDescriptorSetLayout        _materialDescriptorSetLayout{ VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> _materialDescriptorSets{};
};
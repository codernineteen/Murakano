#include "Material.h"

GLTFMetallicRoughnessPipeline::GLTFMetallicRoughnessPipeline(MKDevice& device)
	:
	_deviceRef(device),
	_opaquePipeline(device, "opaque material"),
	_transparentPipeline(device, "transparent material")
{
}

GLTFMetallicRoughnessPipeline::~GLTFMetallicRoughnessPipeline()
{
	// destroy resources
	if (_materialDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(_deviceRef.GetDevice(), _materialDescriptorSetLayout, nullptr);
	}
}

void GLTFMetallicRoughnessPipeline::CreateDescriptorSet(uint32 textureSize)
{
	// local descriptor set (set index 1)
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // material data
		VK_SHADER_VS_FS_FLAG,
		EShaderBinding::UNIFORM_BUFFER,
		1
	);
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, // textures
		VK_SHADER_VS_FS_FLAG,
		EShaderBinding::TEXTURE_2D_ARRAY,
		textureSize
	);
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_SAMPLER, // texture sampler
		VK_SHADER_VS_FS_FLAG,
		EShaderBinding::SAMPLER,
		1
	);

	GDescriptorManager->CreateDescriptorSetLayout(_materialDescriptorSetLayout);
	GDescriptorManager->AllocateDescriptorSet(_materialDescriptorSets, _materialDescriptorSetLayout);
}

MaterialInstance GLTFMetallicRoughnessPipeline::WriteMaterial(EMaterialPass matPass, const MaterialResources& resources)
{
	MaterialInstance materialData{};
	materialData.passType = matPass;
	materialData.pPipeline = (matPass == EMaterialPass::TRANSPARENT) ? _transparentPipeline.GetPipelinePtr() : _opaquePipeline.GetPipelinePtr();
	materialData.setCount = static_cast<uint32>(_materialDescriptorSets.size());
	materialData.materialDescriptorSet = _materialDescriptorSets.data();


	// per-frame descirptor write
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		// write data buffer descriptor (uniform), TODO : per frame uniform buffer
		GDescriptorManager->WriteBufferToDescriptorSet(
			resources.dataBuffer.buffer,          // uniform buffer
			resources.dataBufferOffset,           // offset
			sizeof(MaterialConstants),            // range
			EShaderBinding::UNIFORM_BUFFER,       // binding point
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER     // descriptor type
		);

		// write image array descriptor (texture array 2D)
		std::vector<VkDescriptorImageInfo> imageInfos;
		for (size_t tex_it = 0; tex_it < resources.textureViews.size(); tex_it++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView   = resources.textureViews[tex_it];
			imageInfo.sampler     = nullptr;
			imageInfos.push_back(imageInfo);
		}
		GDescriptorManager->WriteImageArrayToDescriptorSet(
			imageInfos.data(),
			static_cast<uint32>(imageInfos.size()),
			EShaderBinding::TEXTURE_2D_ARRAY,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		);

		// write sampler descriptor
		GDescriptorManager->WriteSamplerToDescriptorSet(
			resources.textureSampler,
			EShaderBinding::SAMPLER,
			VK_DESCRIPTOR_TYPE_SAMPLER
		);

		// update descriptor set
		GDescriptorManager->UpdateDescriptorSet(_materialDescriptorSets[it]);
	}

	return materialData;
}

void GLTFMetallicRoughnessPipeline::BuildPipeline(
	const std::vector<VkPushConstantRange> pcRanges, 
	VkDescriptorSetLayout& globalDescLayout, 
	std::vector<VkFormat> colorAttachmentFormat, 
	VkFormat depthAttachmentFormat
)
{
	// Stencil format is not supported

	// ------------------ opaque pipeline ------------------
	_opaquePipeline.AddShader("../../../shaders/output/spir-v/vertex.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
	_opaquePipeline.AddShader("../../../shaders/output/spir-v/fragment.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
	_opaquePipeline.SetDefaultPipelineCreateInfo();

	// descriptor set layouts
	std::vector<VkDescriptorSetLayout> layouts{ globalDescLayout, _materialDescriptorSetLayout };
	// push constants
	std::vector<VkPushConstantRange> pcRangeCopy = pcRanges;
	
	// create pipeline layout
	_opaquePipeline.AddDescriptorSetLayouts(layouts);
	_opaquePipeline.AddPushConstantRanges(pcRangeCopy);
	_opaquePipeline.CreatePipelineLayout();

	// build pipeline
	_opaquePipeline.SetRenderingInfo(
		static_cast<uint32>(colorAttachmentFormat.size()), 
		colorAttachmentFormat.data(),
		depthAttachmentFormat,
		VK_FORMAT_UNDEFINED
	);
	_opaquePipeline.BuildPipeline();

	// ------------------ transparent pipeline ------------------
	_transparentPipeline.AddShader("../../../shaders/output/spir-v/vertex.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
	_transparentPipeline.AddShader("../../../shaders/output/spir-v/fragment.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
	_transparentPipeline.SetDefaultPipelineCreateInfo();

	_transparentPipeline.AddDescriptorSetLayouts(layouts);
	_transparentPipeline.AddPushConstantRanges(pcRangeCopy);
	_transparentPipeline.CreatePipelineLayout();

	_transparentPipeline.SetRenderingInfo(
		static_cast<uint32>(colorAttachmentFormat.size()),
		colorAttachmentFormat.data(),
		depthAttachmentFormat,
		VK_FORMAT_UNDEFINED
	);
	_transparentPipeline.EnableBlendingAdditive();
	_transparentPipeline.DisableDepthTest();
	_transparentPipeline.BuildPipeline();
}

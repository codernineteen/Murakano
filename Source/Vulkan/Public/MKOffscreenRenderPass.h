#include "Types.h"
#include "Utilities.h"
#include "MKDevice.h"
#include "MKCommandService.h"

class MKOffscreenRenderPass 
{
public:
	MKOffscreenRenderPass(MKDevice* mkDevicePtr);
	~MKOffscreenRenderPass();

	
	void CreateOffscreenRenderPass(VkExtent2D extent);

	/* getters */
	VkRenderPass GetRenderPass() const { return _renderPass; }
	VkDescriptorImageInfo GetColorDesc() const { return _colorDesc; }
	VkDescriptorImageInfo GetDepthDesc() const { return _depthDesc; }
	VkImageAllocated GetColorImage() const { return _colorImage; }
	VkImageView GetColorImageView() const { return _colorImageView; }
	VkSampler GetColorSampler() const { return _colorSampler; }
	VkImageAllocated GetDepthImage() const { return _depthImage; }
	VkImageView GetDepthImageView() const { return _depthImageView; }
	VkSampler GetDepthSampler() const { return _depthSampler; }
	VkFramebuffer GetFramebuffer() const { return _framebuffer; }

private:
	VkRenderPass _renderPass;
	MKDevice* _mkDevicePtr;

	VkDescriptorImageInfo _colorDesc{};
	VkDescriptorImageInfo _depthDesc{};

	VkImageAllocated  _colorImage;
	VkImageView       _colorImageView;
	VkSampler         _colorSampler;

	VkImageAllocated  _depthImage;
	VkImageView       _depthImageView;
	VkSampler         _depthSampler;

	VkFramebuffer     _framebuffer;
};					  
# class naming convention

# 0. General rule
## 0.1 block scope convention
- All scopes should be start next line of the statement.
- ex)
	```cpp
		if (condition)
		{
			// ...
		}
	```

## 0.2 comment convetion
- If it is a single line comment in `*.cpp` files, use `//`
- If it is a single line comment in `*.h` files, use `/**/`
- If it is a multi line comment, follow below format
- ex)
	```cpp
		/**
		* Comment something
		*/
	```
- If it is a instruction commnet, follow below format
- ex)
	```cpp
		int num = 0; // assign 0 to num variable
	```

# 1. Class convention

## 1.0 class naming convention
- All of API specific class should be named with a prefix 'MK' to distinguish from vulkan class.
- ex)
	```cpp
	class MKInstance
	{
		// ...
	};
	```
- On the other hand, Any other class can be named without prefix.
- ex)
	```cpp
	class Allocator
	{
		// ...
	};
	```

## 1.1 member variable naming convention
0. private member		
A private member should be prefixed with '_' (underscore)

- ex)
	```cpp
	private:
		doulbe _privateMember;
	```

1. vulkan prefix
All of class members should be prefixed with 'vk'

- ex)
	```cpp
	private:
		VkInstance _vkInstance;
	```

2. murakano class prefix				
An instance of Murakano-specific class should be prefixed with 'mk'
 
- ex)
	```cpp
	private:
		MKInstance _mkInstance;
	```


## 1.2 member function naming convention

Functions should be named in camel case with the first letter being uppercase.

- ex)
	```cpp
	public:
		void CreateInstance();
	```

But there are some exceptions.
First, If it is a function to get a proxy address of extension function and return it, i'm going to follow the naming convention of vulkan.
- ex)
	```cpp
	public:
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR(...);
	```

## 1.3 class comment convention
- If there is a need to specify the role of certain group of members, follow below convention

- ex)
	```cpp
		/* states */
		bool isValid;
		int  count;
	```


## 1.4 member function parameter space convention
- If a function has number of parameters less than three, write the signature in one line.
- If a function has number of parameters more than three, Follow below formats
	```cpp
		void MKCommandService::SubmitCommandBufferToQueue(
			uint32_t currentFrame, 
			VkSemaphore waitSemaphores[], 
			VkPipelineStageFlags waitStages[], 
			VkSemaphore signalSemaphores[], 
			VkQueue loadedQueue, 
			VkFence fence = nullptr
		);
	```

## 1.5 class member space matching convention.
- When there are multiple members under an access modifier, a space of longest type name has a single space between type and its identifier.
The rest of members follow the space of the member whose type name is the longest.
- ex)
	```cpp
		class MKCommandService
		{
		public:
			VkCommandbuffer _commandBuffer;
			VkCommandPool   _commandPool;
		};
	```


- function spacing follow same rules, but the rule applied on each category.
If a group of function is const, there should be space mathcing of them either.
- ex)
	```cpp
	/* getters */
	VkSwapchainKHR  GetSwapchain()			            const { return _vkSwapchain; }
	VkFormat	    GetSwapchainImageFormat()           const { return _vkSwapchainImageFormat; }
	VkExtent2D	    GetSwapchainExtent()	            const { return _vkSwapchainExtent; }
	VkFramebuffer   GetFramebuffer(uint32_t imageIndex) const { return _vkSwapchainFramebuffers[imageIndex]; }
	VkRenderPass    RequestRenderPass()                 const { return _mkRenderPassPtr->GetRenderPass(); }

	/* setters */
	void		   SetFrameBufferResized(bool isResized) { _framebufferResized = isResized; }
	```

## 1.6 a member function implementation
- If a member function can be implemented in a single line, implement it in header file.
- ex)
	```cpp
		/* getters */
		VkSwapchainKHR  GetSwapchain() const { return _vkSwapchain; }
	```
- If not, implement the function in separate cpp file which included the header file holding the signature of it.

# 2. namespace convention

## 2.0 namespace role
- namespaces in Murakano project should be used to group utility functions if and only if the functions shares similar role.
- ex)
	```cpp
	namespace mk
	{
		namespace vk
		{
			void CreateImage();
			void CreateImageView();
		}
	}
	```

## 2.2 namespace naming convention
- All of the namespace should be scoped inside 'mk' namespace
- ex)
	```cpp
	namespace mk
	{
		// ...
		
		namespace image
		{
			//... image uitilites 
		}
	}
	```


# 3. enum convention
- All of the enum definition should starts with capital letter with a prefix 'E'.
- ex)
	```cpp
	enum EResult
	{
		SUCCESS,
		FAIL,
	};
	```
- Variants of enum should be named in uppercase and separated by underscore.
- ex)
	```cpp
	enum ERayTracingShaderType
	{
		RAYGEN,
		MISS,
		CLOSEST_HIT,
		ANY_HIT,
	};
	```


# 4. global object convention
- All of name of the global objects should starts with a prefix 'G'
- ex)
	```cpp
	class Allocator { /* some code */}

	Allocator* GAllocator; // starts with prefix 'G'
	```
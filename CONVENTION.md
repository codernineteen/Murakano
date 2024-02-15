# class naming convention

# 1. Class convention

## 1.1 member variable
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
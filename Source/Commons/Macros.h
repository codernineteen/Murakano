#pragma once

#include <fmt/format.h>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

// validation macros ( use do-while(0) to avoid a problem that may happen while macro expansion )
#define MK_CHECK(x)                                                                        \
  do 																					   \
  {																						   \
	  VkResult err = x;																	   \
	  if (err)										                                       \
	  {																					   \
		  auto msg = fmt::format("[Murakano says] : vulakn error occured - {}\n", string_VkResult(err));  \
		  throw std::runtime_error(msg);															   \
	  }																					   \
  } while(0)      

// log macros
#define MK_LOG(msg)                                                                        \
  fmt::print("[Murakano says] : {}\n", msg);

// throw macros
#define MK_THROW(msg)                                                                      \
  throw std::runtime_error(fmt::format("[Murakano says] : {}\n", msg));                               											   																				   
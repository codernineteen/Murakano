#pragma once

#include <stdexcept>


template<typename From, typename To>
class Into
{
private:
	using Small = __int8;
	using Big = __int32;

	static Small Test(const To&) { return 0; } // return small type
	static Big Test(...) { return 0; } // return Big type.
	static From MakeFrom() { return 0; } // return From type.

public:
	enum
	{
		exists = sizeof(Test(MakeFrom()) == sizeof(Small)) // If Test(const To&) can be called with MakeFrom(), then conversion exists.
	};
};

template<typename From, typename To>
static To SafeStaticCast(From fromType)
{
	constexpr bool isConvertible = Into<From, To>::exists;

	if (isConvertible)
		return static_cast<To>(fromType);
	else
		throw std::runtime_error("Conversion not possible");
}

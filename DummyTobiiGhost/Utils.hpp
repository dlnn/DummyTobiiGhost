#pragma once
#include <codecvt>
#include <locale>
#include <string>
#include <comdef.h>

namespace Utils
{
#include <Unknwnbase.h>

	template <typename T>
	static void SafeRelease(T*& ptr)
	{
		if (ptr != nullptr)
		{
			reinterpret_cast<IUnknown*>(ptr)->Release();
			ptr = nullptr;
		}
	}

	// 左值特化
	template <typename T>
	static void SafeReleaseArgsImpl(T& arg)
	{
		constexpr auto isBaseOfIUnknowPointer = std::is_base_of_v<IUnknown, std::remove_pointer_t<T>>;
		constexpr auto isArrayOfIUnknown      = std::is_base_of_v<IUnknown, std::remove_pointer_t<std::remove_extent_t<T>>>;

		if constexpr (isBaseOfIUnknowPointer)
		{
			Utils::SafeRelease(arg);
		}
		else if constexpr (isArrayOfIUnknown)
		{
			for (auto& element : arg)
			{
				Utils::SafeRelease(element);
			}
		}
		else
		{
			// 非IUnknown类型 不处理
		}
	}

	// 右值特化
	template <typename T>
	static void SafeReleaseArgsImpl(T&& arg)
	{
		// 防止参数里有宏直接就是值类型
	}

	template <typename... Args>
	static void SafeReleaseArgs(Args&&... args)
	{
		(..., SafeReleaseArgsImpl(std::forward<Args>(args)));
	}

	static std::string HrToString(HRESULT hr)
	{
		_com_error err(hr);
#ifdef UNICODE
		std::wstring                                     wstr(err.ErrorMessage());
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
#else
        return std::string(err.ErrorMessage());
#endif
	}
}

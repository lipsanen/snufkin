#pragma once

#include "patterns.hpp"
#include <future>

namespace MemUtils {
	using namespace patterns;

	bool GetModuleInfo(const std::wstring& moduleName, void** moduleHandle, void** moduleBase, size_t* moduleSize);
	bool GetModuleInfo(void* moduleHandle, void** moduleBase, size_t* moduleSize);
	std::wstring GetModulePath(void* moduleHandle);
	std::vector<void*> GetLoadedModules();
	void* GetSymbolAddress(void* moduleHandle, const char* functionName);

	inline uintptr_t find_pattern(const void* start, size_t length, const PatternWrapper& pattern) {
		if (length < pattern.length())
			return 0;

		auto p = static_cast<const uint8_t*>(start);
		for (auto end = p + length - pattern.length(); p <= end; ++p) {
			if (pattern.match(p))
				return reinterpret_cast<uintptr_t>(p);
		}

		return 0;
	}

	std::vector<MatchedPattern> find_all_sequences(const void* start, size_t length, PatternWrapper* wrapper, size_t ptnIndex);

	template<typename T>
	inline void MarkAsExecutable(T addr) {
		MarkAsExecutable(reinterpret_cast<void*>(addr));
	}
	void MarkAsExecutable(void* addr);

	void ReplaceBytes(void* addr, size_t length, const uint8_t* newBytes);
	void* HookVTable(void** vtable, size_t index, const void* function);

	void AddSymbolLookupHook(void* moduleHandle, void* original, void* target);
	void RemoveSymbolLookupHook(void* moduleHandle, void* original);
	void* GetSymbolLookupResult(void* handle, void* original);

	template<typename T>
	struct identity {
		typedef T type;
	};

	namespace detail {
		void Intercept(const std::wstring& moduleName, size_t n, const std::pair<void**, void*> funcPairs[]);
		void RemoveInterception(const std::wstring& moduleName, size_t n, void** const functions[]);

		template<typename FuncType, size_t N>
		inline void Intercept(const std::wstring& moduleName,
		                      std::array<std::pair<void**, void*>, N>& funcPairs,
		                      FuncType& target,
		                      typename identity<FuncType>::type detour) {
			funcPairs[N - 1] = {reinterpret_cast<void**>(&target), reinterpret_cast<void*>(detour)};
			Intercept(moduleName, N, funcPairs.data());
		}

		template<typename FuncType, size_t N, typename... Rest>
		inline void Intercept(const std::wstring& moduleName,
		                      std::array<std::pair<void**, void*>, N>& funcPairs,
		                      FuncType& target,
		                      typename identity<FuncType>::type detour,
		                      Rest&... rest) {
			funcPairs[N - (sizeof...(rest) / 2 + 1)] = {reinterpret_cast<void**>(&target),
			                                            reinterpret_cast<void*>(detour)};
			Intercept(moduleName, funcPairs, rest...);
		}
	} // namespace detail

	template<typename FuncType>
	inline void Intercept(const std::wstring& moduleName,
	                      FuncType& target,
	                      typename identity<FuncType>::type detour) {
		const std::pair<void**, void*> temp[] = {
		    {reinterpret_cast<void**>(&target), reinterpret_cast<void*>(detour)}};
		detail::Intercept(moduleName, 1, temp);
	}

	template<typename FuncType, typename... Rest>
	inline void Intercept(const std::wstring& moduleName,
	                      FuncType& target,
	                      typename identity<FuncType>::type detour,
	                      Rest&... rest) {
		std::array<std::pair<void**, void*>, sizeof...(rest) / 2 + 1> funcPairs;
		funcPairs[0] = {reinterpret_cast<void**>(&target), reinterpret_cast<void*>(detour)};
		detail::Intercept(moduleName, funcPairs, rest...);
	}

	template<typename... FuncType>
	inline void RemoveInterception(const std::wstring& moduleName, FuncType&... functions) {
		void** const temp[] = {reinterpret_cast<void**>(&functions)...};
		detail::RemoveInterception(moduleName, sizeof...(functions), temp);
	}
} // namespace MemUtils

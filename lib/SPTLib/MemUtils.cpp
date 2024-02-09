#include "sptlib-stdafx.hpp"

#include "MemUtils.hpp"
#include "sptlib.hpp"

namespace MemUtils {
	template<class T>
	class LazilyConstructed {
		// Not a unique_ptr to prevent any further "constructors haven't run yet" issues.
		// _Hopefully_ this gets put in BSS so it's nullptr by default for global objects.
		T* object;

	public:
		LazilyConstructed() : object(nullptr) {}
		// The object is leaked (this is meant to be used only for global stuff anyway).
		// Some stuff calls dlclose after the destructors have been run, so destroying the
		// object here leads to crashes.
		~LazilyConstructed() {}

		T& get() {
			if (!object)
				object = new T();

			assert(object);
			return *object;
		}
	};

	std::vector<MatchedPattern> find_all_sequences(const void* start, size_t length, PatternWrapper* patterns, size_t ptnIndex) {
		std::vector<MatchedPattern> out;
		const void* currentStart = start;
		size_t currentLength = length;
		uintptr_t address;
		do {
			address = find_pattern(currentStart, currentLength, patterns[ptnIndex]);
			if (address) {
				MatchedPattern match;
				//match.ptnIndex = pattern - begin;
				match.ptr = address;

				out.push_back(match);
				currentStart = reinterpret_cast<void*>(address + 1);
				currentLength = length
					- (reinterpret_cast<uintptr_t>(currentStart)
						- reinterpret_cast<uintptr_t>(start));
			}
		} while (address);
		return out;
	}

	static LazilyConstructed<std::unordered_map<void*, std::unordered_map<void*, void*>>> symbolLookupHooks;
	// Mutex happens to default to all zeros as far as I can tell, at least in libstdc++.
	static std::mutex symbolLookupHookMutex;

	void* HookVTable(void** vtable, size_t index, const void* function) {
		auto oldFunction = vtable[index];

		ReplaceBytes(&(vtable[index]), sizeof(void*), reinterpret_cast<uint8_t*>(&function));

		return oldFunction;
	}

	void AddSymbolLookupHook(void* moduleHandle, void* original, void* target) {
		if (!original)
			return;

		std::lock_guard<std::mutex> lock(symbolLookupHookMutex);
		symbolLookupHooks.get()[moduleHandle][original] = target;
	}

	void RemoveSymbolLookupHook(void* moduleHandle, void* original) {
		if (!original)
			return;

		std::lock_guard<std::mutex> lock(symbolLookupHookMutex);
		symbolLookupHooks.get()[moduleHandle].erase(original);
	}

	// This can get called before the global constructors have run.
	void* GetSymbolLookupResult(void* handle, void* original) {
		if (!original)
			return nullptr;

		std::lock_guard<std::mutex> lock(symbolLookupHookMutex);
		auto hook = symbolLookupHooks.get()[handle].find(original);
		if (hook == symbolLookupHooks.get()[handle].end())
			return original;
		else
			return hook->second;
	}
} // namespace MemUtils

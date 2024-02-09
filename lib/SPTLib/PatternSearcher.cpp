#include "tier0/dbg.h"
#include "SPTLib/PatternSearcher.hpp"
#include "SPTLib/sptlib.hpp"
#include "SPTLib/DetoursUtils.hpp"
#include "SPTLib/MemUtils.hpp"

void HookData::UnhookModule(const std::string& moduleName) {
	auto& module = moduleHookData[moduleName];

	if (!module.hookedFunctions.empty())
		DetoursUtils::DetachDetours(module.moduleName,
		                            module.hookedFunctions.size(),
		                            &module.hookedFunctions[0]);

	for (auto& vft_hook : module.existingVTableHooks) {
		if (vft_hook.useVftableHook == UseVFTableHook::True) {
			DevMsg("Hooking %p via the vftable\n", vft_hook.vftable[vft_hook.index]);
			MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, *vft_hook.origPtr);
		}
	}
}

void HookData::InitModule(const std::string& moduleName, patterns::PatternCache* cache) {
	auto& module = moduleHookData[moduleName];
	void* handle;
	void* moduleStart;
	size_t moduleSize;

	if (MemUtils::GetModuleInfo(module.moduleName, &handle, &moduleStart, &moduleSize)) {
		DevMsg("Hooking %s (start: %p; size: %x)...\n", moduleName.c_str(), moduleStart, moduleSize);
	} else {
		DevMsg("Couldn't hook %s, not loaded\n", moduleName.c_str());
		return;
	}

	std::vector<patterns::PatternWrapper*> hooks;
	std::vector<std::vector<patterns::MatchedPattern>> mhooks;
	hooks.reserve(module.patternHooks.size());
	mhooks.reserve(module.matchAllPatterns.size());

	for (auto& mpattern : module.matchAllPatterns) {
		std::vector<patterns::MatchedPattern> result;
		for (size_t i = 0; i < mpattern.size; ++i) {
			auto cachedResult = cache->CheckMatchAllCache(moduleName, moduleStart, mpattern.patternArr, i);
			if (cachedResult.result == patterns::CacheResultEnum::Found) {
				result.insert(result.end(), cachedResult.ptr_vec.begin(), cachedResult.ptr_vec.end());
			}
			else {
				patterns::MatchAllCacheResult newCacheResult;
				newCacheResult.ptr_vec = MemUtils::find_all_sequences(moduleStart, moduleSize, mpattern.patternArr, i);
				newCacheResult.result = patterns::CacheResultEnum::Found;
				cache->AddToMatchAllCache(moduleName, moduleStart, newCacheResult, mpattern.patternArr, i);

				result.insert(result.end(), newCacheResult.ptr_vec.begin(), newCacheResult.ptr_vec.end());
			}
		}
		mhooks.push_back(result);
	}

	for (auto& pattern : module.patternHooks) {
		bool found = false;
		for (size_t i = 0; i < pattern.size; ++i) {
			auto* item = pattern.patternArr + i;
			auto result = cache->CheckCache(moduleName, moduleStart, item);
			if (result.result == patterns::CacheResultEnum::Found) {
				hooks.push_back(item);
				*pattern.origPtr = result.ptr;
				found = true;
				break;
			}
			else if (result.result == patterns::CacheResultEnum::NotCached) {
				auto result = MemUtils::find_pattern(moduleStart, moduleSize, *item);
				patterns::CacheResult cacheResult;

				if (result != 0) {
					cacheResult.result = patterns::CacheResultEnum::Found;
					*pattern.origPtr = cacheResult.ptr = (void*)result;
					hooks.push_back(item);
					found = true;
					cache->AddToCache(moduleName, moduleStart, cacheResult, item);
					break;
				}
				else {
					cacheResult.result = patterns::CacheResultEnum::CachedNotFound;
					cacheResult.ptr = NULL;
					cache->AddToCache(moduleName, moduleStart, cacheResult, item);
				}

			}
		}

		if (!found) {
			hooks.push_back(nullptr);
		}
	}

	module.funcPairs.reserve(module.funcPairs.size() + module.patternHooks.size());
	module.hookedFunctions.reserve(module.hookedFunctions.size() + module.patternHooks.size());

	for (std::size_t i = 0; i < mhooks.size(); ++i) {
		auto modulePattern = module.matchAllPatterns[i];
		*modulePattern.foundVec = std::move(mhooks[i]);
		DevMsg("[%s] Found %u instances of pattern %s\n",
		       moduleName.c_str(),
		       modulePattern.foundVec->size(),
		       modulePattern.patternName);
	}

	for (std::size_t i = 0; i < hooks.size(); ++i) {
		auto foundPattern = hooks[i];
		auto modulePattern = module.patternHooks[i];

		if (*modulePattern.origPtr) {
			if (modulePattern.functionHook) {
				module.funcPairs.emplace_back(modulePattern.origPtr, modulePattern.functionHook);
				module.hookedFunctions.emplace_back(modulePattern.origPtr);
			}

			DevMsg("[%s] Found %s at %p (using the %s pattern).\n",
				moduleName.c_str(),
			       modulePattern.patternName,
			       *modulePattern.origPtr,
			       foundPattern->name());
			patternIndices[reinterpret_cast<uintptr_t>(modulePattern.origPtr)] =
			    foundPattern - modulePattern.patternArr;
		} else {
			DevWarning("[%s] Could not find %s.\n", moduleName.c_str(), modulePattern.patternName);
		}
	}

	for (auto& vft_hook : module.vftableHooks) {
		if (vft_hook.useVftableHook == UseVFTableHook::False) {
			*vft_hook.origPtr = vft_hook.vftable[vft_hook.index];
			module.funcPairs.push_back(std::pair<void**, void*>(vft_hook.origPtr, vft_hook.functionToHook));
			module.hookedFunctions.emplace_back(vft_hook.origPtr);
			DevMsg("Added non vftable hook at %p via vftable\n", *vft_hook.origPtr);
		}
	}

	for (auto& offset : module.offsetHooks) {
		*offset.origPtr = reinterpret_cast<char*>(moduleStart) + offset.offset;

		DevMsg("[%s] Found %s at %p via a fixed offset.\n",
		       Convert(moduleName).c_str(),
		       offset.patternName,
		       *offset.origPtr);

		if (offset.functionHook) {
			module.funcPairs.emplace_back(offset.origPtr, offset.functionHook);
			module.hookedFunctions.emplace_back(offset.origPtr);
		}
	}
}

void HookData::HookModule(const std::string& moduleName) {
	auto& module = moduleHookData[moduleName];
	if (!module.vftableHooks.empty()) {
		for (auto& vft_hook : module.vftableHooks) {
			if (vft_hook.useVftableHook == UseVFTableHook::True) {
				*vft_hook.origPtr = vft_hook.vftable[vft_hook.index];
				MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, vft_hook.functionToHook);
			}
		}
	}

	if (!module.funcPairs.empty()) {
		for (auto& entry : module.funcPairs)
			MemUtils::MarkAsExecutable(*(entry.first));

		DetoursUtils::AttachDetours(module.moduleName, module.funcPairs.size(), &module.funcPairs[0]);
	}

	// Clear any hooks that were added
	module.offsetHooks.clear();
	module.patternHooks.clear();
	// VTable hooks have to be stored for the unhooking code
	module.existingVTableHooks.insert(module.existingVTableHooks.end(),
	                                  module.vftableHooks.begin(),
	                                  module.vftableHooks.end());
	module.vftableHooks.clear();
}

ModuleHookData& HookData::getHookData(const std::string& moduleName) {
	if (moduleHookData.find(moduleName) == moduleHookData.end()) {
		moduleHookData[moduleName] = ModuleHookData(moduleName);
	}
	return moduleHookData[moduleName];
}

ModuleHookData::ModuleHookData(const std::string& moduleNameCstr) { moduleName = Convert(moduleNameCstr); }

VFTableHook::VFTableHook(void** vftable,
                         int index,
                         void* functionToHook,
                         void** origPtr,
                         UseVFTableHook useVftableHook) {
	this->vftable = vftable;
	this->index = index;
	this->functionToHook = functionToHook;
	this->origPtr = origPtr;
	this->useVftableHook = useVftableHook;
}

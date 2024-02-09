#pragma once

#include "patterns.hpp"
#include "SPTLib/PatternCacher.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct PatternHook {
	PatternHook(patterns::PatternWrapper* patternArr,
	            size_t size,
	            const char* patternName,
	            void** origPtr,
	            void* functionHook) {
		this->patternArr = patternArr;
		this->size = size;
		this->patternName = patternName;
		this->origPtr = origPtr;
		this->functionHook = functionHook;
	}

	patterns::PatternWrapper* patternArr;
	size_t size;
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct MatchAllPattern {
	MatchAllPattern(patterns::PatternWrapper* patternArr,
	                size_t size,
	                const char* patternName,
	                std::vector<patterns::MatchedPattern>* foundVec) {
		this->patternArr = patternArr;
		this->size = size;
		this->patternName = patternName;
		this->foundVec = foundVec;
	}

	patterns::PatternWrapper* patternArr;
	size_t size;
	const char* patternName;
	std::vector<patterns::MatchedPattern>* foundVec;
};

enum class UseVFTableHook { False, True };

struct VFTableHook {
	VFTableHook(void** vftable, int index, void* functionToHook, void** origPtr, UseVFTableHook useVftableHook=UseVFTableHook::False);

	void** vftable;
	int index;
	void* functionToHook;
	void** origPtr;
	UseVFTableHook useVftableHook;
};

struct OffsetHook {
	int32_t offset;
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct RawHook {
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct ModuleHookData {
    ModuleHookData(const std::string& moduleNameCstr);
    ModuleHookData() = default;

    std::wstring moduleName;
	std::vector<PatternHook> patternHooks;
	std::vector<MatchAllPattern> matchAllPatterns;
	std::vector<VFTableHook> vftableHooks;
	std::vector<OffsetHook> offsetHooks;

	std::vector<std::pair<void**, void*>> funcPairs;
	std::vector<void**> hookedFunctions;
	std::vector<VFTableHook> existingVTableHooks;
};

struct HookData {
    std::unordered_map<std::string, ModuleHookData> moduleHookData;
    std::unordered_map<uintptr_t, int> patternIndices;
    ModuleHookData& getHookData(const std::string& moduleName);
	void InitModule(const std::string& moduleName, patterns::PatternCache* cacher);
	void HookModule(const std::string& moduleName);
	void UnhookModule(const std::string& moduleName);
};


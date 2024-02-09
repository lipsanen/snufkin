#include <future>
#include "feature.hpp"
#include "SPTLib/sptlib.hpp"
#include "SPTLib/PatternSearcher.hpp"

#include "SPTLib/Hooks.hpp"
#include "tier0/dbg.h"

static HookData hookData;
static std::unordered_map<uintptr_t, int>& patternIndices = hookData.patternIndices;
static std::unordered_map<std::string, ModuleHookData>& moduleHookData = hookData.moduleHookData;

static std::vector<Feature*>& GetFeatures() {
	static std::vector<Feature*> features;
	return features;
}

void Feature::LoadFeatures(patterns::PatternCache* cache) {
	// This is a restart, reload the features
	auto start = std::chrono::steady_clock::now();
	Hooks::InitInterception(true);
	auto end = std::chrono::steady_clock::now();
	DevMsg("Init interception took %f milliseconds\n", (end - start).count() / 1e6);

	start = std::chrono::steady_clock::now();
	for (auto feature : GetFeatures()) {
		if (!feature->moduleLoaded && feature->ShouldLoadFeature()) {
			feature->startedLoading = true;
			feature->InitHooks();
		}
	}
	end = std::chrono::steady_clock::now();
	DevMsg("Init hooks took %f milliseconds\n", (end - start).count() / 1e6);

	start = std::chrono::steady_clock::now();
	InitModules(cache);
	end = std::chrono::steady_clock::now();
	DevMsg("Finding patterns took %f milliseconds\n", (end - start).count() / 1e6);


	start = std::chrono::steady_clock::now();
	for (auto feature : GetFeatures()) {
		if (!feature->moduleLoaded && feature->startedLoading) {
			feature->PreHook();
		}
	}
	end = std::chrono::steady_clock::now();
	DevMsg("Prehook code took %f milliseconds\n", (end - start).count() / 1e6);

	start = std::chrono::steady_clock::now();
	Hook();
	end = std::chrono::steady_clock::now();
	DevMsg("Hooking code took %f milliseconds\n", (end - start).count() / 1e6);

	for (auto feature : GetFeatures()) {
		if (!feature->moduleLoaded && feature->startedLoading) {
			feature->LoadFeature();
			feature->moduleLoaded = true;
		}
	}
}

void Feature::UnloadFeatures() {
	for (auto feature : GetFeatures()) {
		if (feature->moduleLoaded) {
			feature->UnloadFeature();
			feature->moduleLoaded = false;
		}
	}

	Unhook();

	for (auto feature : GetFeatures()) {
		if (feature->moduleLoaded) {
			feature->~Feature();
		}
	}

	moduleHookData.clear();
	patternIndices.clear();
}

void Feature::AddVFTableHook(VFTableHook hook, std::string moduleEnum) {
	auto& mhd = hookData.getHookData(moduleEnum); 
	mhd.vftableHooks.push_back(hook);
}

Feature::Feature() {
	moduleLoaded = false;
	startedLoading = false;
	GetFeatures().push_back(this);
}

void Feature::InitModules(patterns::PatternCache* cacher) {
	for (auto& pair : moduleHookData) {
		hookData.InitModule(pair.first, cacher);
	}
}

void Feature::Hook() {
	for (auto& pair : moduleHookData) {
		hookData.HookModule(pair.first);
	}
}

void Feature::Unhook() {
	for (auto& pair : moduleHookData) {
		hookData.UnhookModule(pair.first); 
	}
}

void Feature::AddOffsetHook(std::string moduleEnum,
                            int offset,
                            const char* patternName,
                            void** origPtr,
                            void* functionHook) {
	auto& mhd = hookData.getHookData(moduleEnum); 
	mhd.offsetHooks.push_back(OffsetHook{offset, patternName, origPtr, functionHook});
}

int Feature::GetPatternIndex(void** origPtr) {
	uintptr_t ptr = reinterpret_cast<uintptr_t>(origPtr);
	if (patternIndices.find(ptr) != patternIndices.end()) {
		return patternIndices[ptr];
	} else {
		return -1;
	}
}

void Feature::AddRawHook(std::string moduleName, void** origPtr, void* functionHook) {
	auto& mhd = hookData.getHookData(moduleName);
	mhd.funcPairs.emplace_back(origPtr, functionHook);
	mhd.hookedFunctions.emplace_back(origPtr);
}

void Feature::AddPatternHook(PatternHook hook, std::string moduleName) {
	auto& mhd = hookData.getHookData(moduleName);
	mhd.patternHooks.push_back(hook);
}

void Feature::AddMatchAllPattern(MatchAllPattern hook, std::string moduleName) {
	auto& mhd = hookData.getHookData(moduleName);
	mhd.matchAllPatterns.push_back(hook);
}


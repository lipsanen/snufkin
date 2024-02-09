#include "SPTLib/PatternCacher.hpp"
#include "nlohmann/json.hpp"
#include "tier0/dbg.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

using namespace patterns;

#define UNIQUE_KEY "unique"
#define ALL_KEY "all"

// Save this to file if it has changed
void PatternCache::SaveIfChanged(const std::string& filepath)
{
	if (m_bUpdated)
	{
		try {
			nlohmann::json obj;
			obj[UNIQUE_KEY] = m_mOffsets;
			obj[ALL_KEY] = m_mMatchAllOffsets;
			std::ofstream output_file(filepath);
			output_file << obj;
		}
		catch (...) {
			Warning("Writing pattern cache to filepath %s failed!\n", filepath.c_str());
		}
	}
}

static std::string getPatternKey(const std::string& moduleName, const patterns::PatternWrapper* pattern) {
	char key[256];
	snprintf(key, sizeof(key), "%s-%s", moduleName.c_str(), pattern->origPattern);
	return key;
}


// Load from file
void PatternCache::LoadFromFile(const std::string& filepath)
{
	try {
		std::error_code ec;
		if (std::filesystem::exists(std::filesystem::path(filepath), ec)) {
			std::ifstream input_file(filepath);
			nlohmann::json obj;
			input_file >> obj;

			if (obj.find(UNIQUE_KEY) != obj.end()) {
				m_mOffsets = obj[UNIQUE_KEY];
			}

			if (obj.find(ALL_KEY) != obj.end()) {
				m_mMatchAllOffsets = obj[ALL_KEY];
			}
		}
	}
	catch (...) {
		Warning("Loading pattern cache from filepath %s failed!\n", filepath.c_str());
	}
}

// Return pointer to pattern if found, null pointer if not
CacheResult PatternCache::CheckCache(const std::string& moduleName, void* moduleStart, const patterns::PatternWrapper* pattern)
{
	CacheResult result;
	result.ptr = nullptr;

	auto key = getPatternKey(moduleName, pattern);
	std::uint8_t* target;
	auto it = m_mOffsets.find(key);

	if (it == m_mOffsets.end()) {
		result.result = CacheResultEnum::NotCached;
		return result;
	}
	else if (it->second == -1) {
		result.result = CacheResultEnum::CachedNotFound;
		return result;
	}

	// Check if the address matches before using it
	target = it->second + (std::uint8_t*)moduleStart;

	if (pattern->match(target)) {
		result.ptr = target;
		result.result = CacheResultEnum::Found;
		return result;
	}

    result.result = CacheResultEnum::NotCached; // cached result but wrong, treat as if not cached
    return result;
}

// Add result to cache
void PatternCache::AddToCache(const std::string& moduleName, void* moduleStart, CacheResult result, const patterns::PatternWrapper* pattern)
{
	auto key = getPatternKey(moduleName, pattern);
	int offset;

	if (result.result == CacheResultEnum::CachedNotFound) {
		offset = -1;
	}
	else {
		offset = (uint8_t*)result.ptr - (uint8_t*)moduleStart;
	}

	m_mOffsets[key] = offset;
	m_bUpdated = true;
}


// Cache but for match all patterns
MatchAllCacheResult PatternCache::CheckMatchAllCache(const std::string& moduleName, void* moduleStart, patterns::PatternWrapper* patterns, size_t ptnIndex) {
	MatchAllCacheResult result;
	auto key = getPatternKey(moduleName, patterns + ptnIndex);
	auto it = m_mMatchAllOffsets.find(key);

	if (it == m_mMatchAllOffsets.end()) {
		result.result = CacheResultEnum::NotCached;
		return result;
	}

	result.result = CacheResultEnum::Found;
	result.ptr_vec.reserve(it->second.size());

	for (auto offset : it->second) {
		MatchedPattern ptn;
		ptn.ptnIndex = ptnIndex;
		ptn.ptr = (std::uintptr_t)moduleStart + offset;
		result.ptr_vec.push_back(ptn);
	}

	return result;
}

// Add match all to cache
void PatternCache::AddToMatchAllCache(const std::string& moduleName, void* moduleStart, MatchAllCacheResult result, patterns::PatternWrapper* patterns, size_t ptnIndex) {
	auto key = getPatternKey(moduleName, patterns + ptnIndex);
	std::vector<int> vec;
	vec.reserve(result.ptr_vec.size());

	for (auto ptn : result.ptr_vec) {
		vec.push_back((std::uint8_t*)ptn.ptr - (std::uint8_t*)moduleStart);
	}

	m_mMatchAllOffsets[key] = vec;
	m_bUpdated = true;
}

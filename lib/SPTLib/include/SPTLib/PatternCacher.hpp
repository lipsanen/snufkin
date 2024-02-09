#pragma once

#include "patterns.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace patterns {
	enum class CacheResultEnum { NotCached, Found, CachedNotFound };

	struct CacheResult {
		CacheResultEnum result;
		void* ptr;
	};

	struct MatchAllCacheResult {
		CacheResultEnum result;
		std::vector<MatchedPattern> ptr_vec;
	};

	class PatternCache {
	public:
		// Save this to file if it has changed
		void SaveIfChanged(const std::string& filepath);
		// Load from file
		void LoadFromFile(const std::string& filepath);
		// Return pointer to pattern if found, null pointer if not
		CacheResult CheckCache(const std::string& moduleName, void* moduleStart, const patterns::PatternWrapper* pattern);
		// Add result to cache
		void AddToCache(const std::string& moduleName, void* moduleStart, CacheResult result, const patterns::PatternWrapper* pattern);
		// Cache but for match all patterns
		MatchAllCacheResult CheckMatchAllCache(const std::string& moduleName, void* moduleStart, PatternWrapper* patterns, size_t ptnIndex);
		// Add match all to cache
		void AddToMatchAllCache(const std::string& moduleName, void* moduleStart, MatchAllCacheResult result, PatternWrapper* patterns, size_t ptnIndex);
	private:
		std::unordered_map<std::string, int> m_mOffsets;
		std::unordered_map<std::string, std::vector<int>> m_mMatchAllOffsets;
		bool m_bUpdated = false;
	};
}
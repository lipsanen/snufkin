#include "utils/game_detection.hpp"
#include "utils/interfaces.hpp"
#include "icvar.h"
#include "SPTLib/MemUtils.hpp"

#define DECLARE_AND_CHECK_CACHE() \
	static bool cached = false; \
	static bool result = false; \
	if (cached) \
	return result

bool utils::DoesGameLookLikePortal() {
	DECLARE_AND_CHECK_CACHE();
	result = g_pCVar->FindCommand("upgrade_portalgun") != NULL;
	cached = true;
	return result;
}

static int build_num = 0;

namespace patterns {
	PATTERNS(build_num, "pattern",
	         "45 78 65 20 62 75 69 6C 64 3A"); // "Exe build:" in hex
}

static int DateToBuildNumber(const char* date_str) {
	const char* months[] = {
	    "Jan",
	    "Feb",
	    "Mar",
	    "Apr",
	    "May",
	    "Jun",
	    "Jul",
	    "Aug",
	    "Sep",
	    "Oct",
	    "Nov",
	    "Dec",
	};

	int monthDays[] = {
	    31,
	    28,
	    31,
	    30,
	    31,
	    30,
	    31,
	    31,
	    30,
	    31,
	    30,
	    31,
	};

	int m = 0, d = 0, y = 0;

	while (m < 11) {
		auto month = months[m];
		if (strstr(date_str, month) == date_str) {
			break;
		}

		d += monthDays[m];
		m += 1;
	}

	if (date_str[4] == ' ') {
		d += (date_str[5] - '0') - 1;
	} else {
		d += (date_str[4] - '0') * 10 + (date_str[5] - '0') - 1;
	}

	y = std::atoi(date_str + 7) - 1900;

	int build_num;

	build_num = (y - 1) * 365.25;
	build_num += d;
	if (y % 4 == 0 && m > 1)
		build_num += 1;
	build_num -= 35739;
	return build_num;
}

void utils::SearchBuildNumber(patterns::PatternCache* cache) {
	void* handle;
	uint8_t* moduleStart;
	uint8_t* moduleEnd;
	size_t moduleSize;

	if (MemUtils::GetModuleInfo(L"engine.dll", &handle, reinterpret_cast<void**>(&moduleStart), &moduleSize)) {
		moduleEnd = moduleStart + moduleSize;
		const char* BUILD_STRING = "Exe build:";
		auto cacheResult = cache->CheckCache("engine", moduleStart, &patterns::build_num[0]);
		const char* match = NULL;

		if (cacheResult.result == patterns::CacheResultEnum::Found) {
			match = (const char*)cacheResult.ptr;
		} else if (cacheResult.result == patterns::CacheResultEnum::NotCached) {
			match = (const char*)MemUtils::find_pattern(moduleStart, moduleSize, patterns::build_num[0]);
			patterns::CacheResult searchResult;
			searchResult.ptr = (void*)match;
			searchResult.result = match != NULL ? patterns::CacheResultEnum::Found
			                                    : patterns::CacheResultEnum::CachedNotFound;
			cache->AddToCache("engine", moduleStart, searchResult, &patterns::build_num[0]);
		}

		if (match != NULL) {
			DevMsg("Found date string: %s", match);
			build_num = DateToBuildNumber(match + 20);
			DevMsg("Build number is %d\n", build_num);
		} else {
			Warning("Was unable to find date string! Build information not available\n");
		}
	}
}

int utils::GetBuildNumber() { return build_num; }

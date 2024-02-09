#pragma once

#include "SPTLib/PatternCacher.hpp"

// If you call these DoesGameLookLike functions when g_pCVar is null then you crash
namespace utils {
	bool DoesGameLookLikePortal();
	void SearchBuildNumber(patterns::PatternCache* cache);
	int GetBuildNumber();
} // namespace utils

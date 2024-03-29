#pragma once

#include <string>

#define SPTLIB_VERSION "1.2"


#ifndef SPT_MESSAGE_PREFIX
#define SPT_MESSAGE_PREFIX "SPTLib: "
#endif

#define EngineMsg(...) Msg(SPT_MESSAGE_PREFIX __VA_ARGS__)
#define EngineDevMsg(...) DevMsg(SPT_MESSAGE_PREFIX __VA_ARGS__)
#define EngineWarning(...) Warning(SPT_MESSAGE_PREFIX __VA_ARGS__)
#define EngineDevWarning(...) DevWarning(SPT_MESSAGE_PREFIX __VA_ARGS__)

std::wstring::size_type GetRightmostSlash(const std::wstring& str, std::wstring::size_type pos = std::wstring::npos);
std::wstring GetFileName(const std::wstring& fileNameWithPath);
std::wstring GetFolderName(const std::wstring& fileNameWithPath);
std::wstring NormalizePath(const std::wstring& path);
std::wstring Convert(const std::string& from);
std::string Convert(const std::wstring& from);

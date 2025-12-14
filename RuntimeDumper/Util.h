#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <il2cpp-class-internals.h>
#include <il2cpp-object-internals.h>

std::wstring Il2cppToWString(Il2CppString* str);

bool ReplaceIl2CppStringChars(Il2CppString* target, const std::wstring& ws);

Il2CppString* CreateIl2CppString(const std::wstring& ws, Il2CppString* original);

std::string ByteArrayToHex(const uint8_t* data, size_t len);

std::string Utf16ToUtf8(const std::wstring& wstr);

std::wstring Utf8ToUtf16(const std::string& str);

std::string Utf16ToAnsi(const std::wstring& wstr);

std::wstring AnsiToUtf16(const std::string& str);

std::string AsciiEscapeToEscapeLiterals(const std::string& input);

#include "pch.h"
#include "Util.h"
#include <string>
#include <iomanip>
#include <unordered_map>

std::wstring Il2CppToWString(Il2CppString* str) {
    if (!str || str->length <= 0)
        return {};

    return std::wstring(str->chars, str->length);
}

std::string Il2CppToUtf8String(Il2CppString* str) {
    return Utf16ToUtf8(Il2CppToWString(str));
}

Il2CppString* CreateIl2CppString(const std::wstring& ws, Il2CppString* original)
{
    if (!original) return nullptr;

    int32_t len = static_cast<int32_t>(ws.size());
    size_t size = sizeof(Il2CppString) + (len - 1) * sizeof(wchar_t);

    Il2CppString* newStr = (Il2CppString*)malloc(size);
    if (!newStr) return nullptr;

    newStr->object.klass = original->object.klass;
    newStr->object.monitor = nullptr;
    newStr->length = len;

    memcpy(newStr->chars, ws.c_str(), len * sizeof(wchar_t));

    return newStr;
}

bool ReplaceIl2CppStringChars(Il2CppString* target, const std::wstring& ws)
{
    if (!target)
        return false;

    const size_t capacity = static_cast<size_t>(target->length);

    if (ws.size() > capacity)
        return false;

    std::memcpy(target->chars, ws.c_str(), ws.size() * sizeof(wchar_t));

    if (ws.size() < capacity) {
        std::memset(target->chars + ws.size(), 0, (capacity - ws.size()) * sizeof(wchar_t));
    }

    target->length = static_cast<int32_t>(ws.size());
    return true;
}

std::string ByteArrayToHex(const uint8_t* data, size_t len)
{
    if (!data || len == 0) return "";

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
    {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string Utf16ToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return {};

    auto size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(),result.data(), size, NULL, NULL);

    return result;
}

std::wstring Utf8ToUtf16(const std::string& str)
{
    if (str.empty())
        return {};

    auto size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);

    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), size);

    return result;
}

std::string Utf16ToAnsi(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    auto codePage = GetACP();
    auto sizeNeeded = WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string();

    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.size(), &result[0], sizeNeeded, NULL, NULL);

    return result;
}

std::wstring AnsiToUtf16(const std::string& str) {
    if (str.empty()) return std::wstring();

    auto codePage = GetACP();
    auto sizeNeeded = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (sizeNeeded <= 0) return std::wstring();

    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), &result[0], sizeNeeded);

    return result;
}

std::string AsciiEscapeToEscapeLiterals(const std::string& input) {

    std::unordered_map<char, std::string> escapeSequences = {
        {'\a', "\\a"},
        {'\b', "\\b"},
        {'\f', "\\f"},
        {'\n', "\\n"},
        {'\r', "\\r"},
        {'\t', "\\t"},
        {'\"', "\\\""},
        {'\'', "\\'"},
        {'\\', "\\\\"},
        {'\v', "\\v"},
        {'\0', "\\0"},
        {'\x1b', "\\e"},
    };

    std::string result;
    for (char ch : input) {
        auto it = escapeSequences.find(ch);
        if (it != escapeSequences.end()) {
            result += it->second;
        }
        else {
            result += ch;
        }
    }

    return result;
}

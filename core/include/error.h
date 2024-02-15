#pragma once

#include <string_view>

void LogError(std::string_view error_message, std::string_view file, int line);
#define LOG_ERROR(error_message) LogError(error_message, __FILE__, __LINE__);

void CheckError(std::string_view file, int line);
#define GL_CHECK_ERROR() CheckError(__FILE__, __LINE__)
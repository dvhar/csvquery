#pragma once
#include<string>
std::string escapeJSON(const std::basic_string_view<char>& input);
std::string chopAndEscapeJson(std::basic_string_view<char>&& input);

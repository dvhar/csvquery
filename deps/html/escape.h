#pragma once
#include<string>
std::string escapeHTML(const std::basic_string_view<char>& input);
std::string chopAndEscapeHTML(std::basic_string_view<char>&& input);

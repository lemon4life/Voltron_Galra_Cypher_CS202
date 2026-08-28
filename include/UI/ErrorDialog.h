#pragma once

#include <string>

namespace ErrorDialog {

void Show(const std::string& errorText);
std::string GetCurrentExceptionMessage();

} // namespace ErrorDialog

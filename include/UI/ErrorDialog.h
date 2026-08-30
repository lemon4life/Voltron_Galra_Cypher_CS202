#pragma once

#include <string>

namespace ErrorDialog {

/// Displays this UI and handles its interaction until it is dismissed.
void Show(const std::string& errorText);
/// Returns the current current exception message.
std::string GetCurrentExceptionMessage();

} // namespace ErrorDialog

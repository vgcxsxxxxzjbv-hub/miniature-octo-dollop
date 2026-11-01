#pragma once
#include <string>
namespace spinox::codegen {
std::string generateLogHook(void* method);
std::string generateReturnHook(void* method, const char* returnExpr);
std::string generateArgHook(void* method, int argIndex, const char* newExpr);
}

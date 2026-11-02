#pragma once
#include <string>
#include <vector>
namespace spinox::preset {
struct FieldMod {
    std::string classNs, className, fieldName, typeName, valueStr;
    bool isStatic;
};
void saveCurrent(const char* label);
void loadPreset(const char* path);
void applyAll();
std::vector<FieldMod>& mods();
std::string presetDir();
}

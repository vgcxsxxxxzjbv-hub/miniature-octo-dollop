#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace spinox::array_edit {
struct Item { void* ptr; std::string preview; };
bool isArray(void* klass);
bool isList(void* klass);
int length(void* obj);
void* at(void* obj, int i);
std::vector<Item> readAll(void* obj);
std::string elementTypeName(void* klass);
size_t elementSize(void* klass);
}

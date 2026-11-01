#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
namespace spinox::il2cpp {
bool init();
void* getClass(const char* ns, const char* name);
void* getMethod(void* klass, const char* name, int argc);
void* methodPtr(void* method);
const char* className(void* k);
const char* classNamespace(void* k);
void*       classParent(void* k);
size_t      classValueSize(void* k);
bool        classIsValueType(void* k);
void*       classFromType(void* t);
using ClassCb = void(*)(void* k, void* u);
using MethodCb = void(*)(void* m, void* u);
using FieldCb  = void(*)(void* f, void* u);
using InstCb   = void(*)(void* obj, void* u);
void forEachClass(ClassCb cb, void* u);
void forEachMethod(void* k, MethodCb cb, void* u);
void forEachField(void* k, FieldCb cb, void* u);
void forEachInstance(void* k, InstCb cb, void* u);
const char* methodName(void* m);
int         methodArgc(void* m);
void*       methodReturnType(void* m);
void*       methodParamType(void* m, int i);
uint32_t    methodRva(void* m);
void*       runtimeInvoke(void* m, void* obj, void** args, void** exc);
const char* fieldName(void* f);
void*       fieldType(void* f);
size_t      fieldOffset(void* f);
bool        fieldIsStatic(void* f);
void fieldGet(void* obj, void* f, void* out);
void fieldSet(void* obj, void* f, const void* in);
void fieldStaticGet(void* f, void* out);
void fieldStaticSet(void* f, const void* in);
char* typeNameAlloc(void* t);
void  freeString(char* s);
int   typeKind(void* t);
void* typeClass(void* t);
void* objectGetClass(void* o);
std::string readIl2cppString(void* s);
void* makeIl2cppString(const char* s);
}

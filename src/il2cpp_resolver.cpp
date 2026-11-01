#include "spinox/il2cpp.h"
#include "spinox/log.h"
#include <dlfcn.h>
namespace spinox::il2cpp {
using t_dom_get        = void*(*)();
using t_dom_asms       = const void*(*)(void*, size_t*);
using t_asm_get_img    = void*(*)(const void*);
using t_img_get_ccount = size_t(*)(void*);
using t_img_get_class  = void*(*)(void*, size_t);
using t_class_from_name= void*(*)(void*, const char*, const char*);
using t_class_name     = const char*(*)(void*);
using t_class_ns       = const char*(*)(void*);
using t_class_par      = void*(*)(void*);
using t_class_is_vt    = bool(*)(void*);
using t_class_vsz      = int(*)(void*, uint32_t*);
using t_class_from_t   = void*(*)(void*);
using t_class_meths    = void*(*)(void*, void**);
using t_class_fields   = void*(*)(void*, void**);
using t_meth_name      = const char*(*)(void*);
using t_meth_pc        = int(*)(void*);
using t_meth_pt        = void*(*)(void*, int);
using t_meth_ret       = void*(*)(void*);
using t_meth_from_name = void*(*)(void*, const char*, int);
using t_meth_rva       = uint32_t(*)(void*);
using t_fld_name       = const char*(*)(void*);
using t_fld_type       = void*(*)(void*);
using t_fld_off        = size_t(*)(void*);
using t_fld_static     = bool(*)(void*);
using t_fld_get        = void(*)(void*, void*, void*);
using t_fld_set        = void(*)(void*, void*, const void*);
using t_fld_sget       = void(*)(void*, void*);
using t_fld_sset       = void(*)(void*, const void*);
using t_type_name      = char*(*)(void*);
using t_type_enum      = int(*)(void*);
using t_type_class     = void*(*)(void*);
using t_free           = void(*)(void*);
using t_obj_class      = void*(*)(void*);
using t_gc_foreach     = void(*)(void*, void*);
using t_str_len        = int32_t(*)(void*);
using t_str_chars      = uint16_t*(*)(void*);
using t_str_new        = void*(*)(const char*);
using t_invoke         = void*(*)(void*, void*, void**, void**);
static void* g_h = nullptr;
static t_dom_get _dom=nullptr; static t_dom_asms _asms=nullptr;
static t_asm_get_img _img=nullptr; static t_img_get_ccount _ccount=nullptr;
static t_img_get_class _cat=nullptr; static t_class_from_name _cfrom=nullptr;
static t_class_name _cname=nullptr; static t_class_ns _cns=nullptr;
static t_class_par _cpar=nullptr; static t_class_is_vt _isvt=nullptr;
static t_class_vsz _vsz=nullptr; static t_class_from_t _cfromt=nullptr;
static t_class_meths _cmeths=nullptr; static t_class_fields _cfields=nullptr;
static t_meth_name _mname=nullptr; static t_meth_pc _mpc=nullptr;
static t_meth_pt _mpt=nullptr; static t_meth_ret _mret=nullptr;
static t_meth_from_name _mfrom=nullptr; static t_meth_rva _mrva=nullptr;
static t_fld_name _fname=nullptr; static t_fld_type _ftype=nullptr;
static t_fld_off _foff=nullptr; static t_fld_static _fstat=nullptr;
static t_fld_get _fget=nullptr; static t_fld_set _fset=nullptr;
static t_fld_sget _fgets=nullptr; static t_fld_sset _fsets=nullptr;
static t_type_name _tname=nullptr; static t_type_enum _tenum=nullptr;
static t_type_class _tclass=nullptr; static t_free _free=nullptr;
static t_obj_class _ogetc=nullptr; static t_gc_foreach _gcfe=nullptr;
static t_str_len _strlen=nullptr; static t_str_chars _strch=nullptr;
static t_str_new _strnew=nullptr; static t_invoke _invoke=nullptr;
bool init() {
    if (g_h) return true;
    g_h = dlopen("libil2cpp.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!g_h) return false;
    #define S(var, type, name) var = (type)dlsym(g_h, "il2cpp_" #name)
    S(_dom, t_dom_get, domain_get);
    S(_asms, t_dom_asms, domain_get_assemblies);
    S(_img, t_asm_get_img, assembly_get_image);
    S(_ccount, t_img_get_ccount, image_get_class_count);
    S(_cat, t_img_get_class, image_get_class);
    S(_cfrom, t_class_from_name, class_from_name);
    S(_cname, t_class_name, class_get_name);
    S(_cns, t_class_ns, class_get_namespace);
    S(_cpar, t_class_par, class_get_parent);
    S(_isvt, t_class_is_vt, class_is_valuetype);
    S(_vsz, t_class_vsz, class_value_size);
    S(_cfromt, t_class_from_t, class_from_type);
    S(_cmeths, t_class_meths, class_get_methods);
    S(_cfields, t_class_fields, class_get_fields);
    S(_mname, t_meth_name, method_get_name);
    S(_mpc, t_meth_pc, method_get_param_count);
    S(_mpt, t_meth_pt, method_get_param);
    S(_mret, t_meth_ret, method_get_return_type);
    S(_mfrom, t_meth_from_name, class_get_method_from_name);
    S(_mrva, t_meth_rva, method_get_relative_virtual_address);
    S(_fname, t_fld_name, field_get_name);
    S(_ftype, t_fld_type, field_get_type);
    S(_foff, t_fld_off, field_get_offset);
    S(_fstat, t_fld_static, field_is_static);
    S(_fget, t_fld_get, field_get_value);
    S(_fset, t_fld_set, field_set_value);
    S(_fgets, t_fld_sget, field_static_get_value);
    S(_fsets, t_fld_sset, field_static_set_value);
    S(_tname, t_type_name, type_get_name);
    S(_tenum, t_type_enum, type_get_type);
    S(_tclass, t_type_class, type_get_class_or_element_class);
    S(_free, t_free, free);
    S(_ogetc, t_obj_class, object_get_class);
    S(_gcfe, t_gc_foreach, gc_heap_foreach);
    S(_strlen, t_str_len, string_length);
    S(_strch, t_str_chars, string_chars);
    S(_strnew, t_str_new, string_new);
    S(_invoke, t_invoke, runtime_invoke);
    #undef S
    if (!_dom || !_cfrom || !_mfrom) { LOGI("il2cpp symbols missing"); return false; }
    LOGI("il2cpp ready"); return true;
}
void* getClass(const char* ns, const char* name) {
    if (!_dom || !_asms || !_img || !_cfrom) return nullptr;
    size_t n = 0;
    const void* const* a = (const void* const*)_asms(_dom(), &n);
    for (size_t i = 0; i < n; i++) {
        void* im = _img(a[i]); if (!im) continue;
        void* k = _cfrom(im, ns, name); if (k) return k;
    }
    return nullptr;
}
void* getMethod(void* k, const char* n, int c) { return (_mfrom && k) ? _mfrom(k, n, c) : nullptr; }
void* methodPtr(void* m) { return m ? *(void**)m : nullptr; }
const char* className(void* k) { return (_cname && k) ? _cname(k) : ""; }
const char* classNamespace(void* k) { return (_cns && k) ? _cns(k) : ""; }
void* classParent(void* k) { return (_cpar && k) ? _cpar(k) : nullptr; }
size_t classValueSize(void* k) { uint32_t a = 0; return (_vsz && k) ? (size_t)_vsz(k, &a) : 0; }
bool classIsValueType(void* k) { return (_isvt && k) ? _isvt(k) : false; }
void* classFromType(void* t) { return (_cfromt && t) ? _cfromt(t) : nullptr; }
void forEachClass(ClassCb cb, void* u) {
    if (!_dom || !_asms || !_img || !_ccount || !_cat) return;
    size_t n = 0;
    const void* const* a = (const void* const*)_asms(_dom(), &n);
    for (size_t i = 0; i < n; i++) {
        void* im = _img(a[i]); if (!im) continue;
        size_t c = _ccount(im);
        for (size_t j = 0; j < c; j++) {
            void* k = _cat(im, j);
            if (k) cb(k, u);
        }
    }
}
void forEachMethod(void* k, MethodCb cb, void* u) {
    if (!k || !_cmeths) return;
    void* it = nullptr;
    while (void* m = _cmeths(k, &it)) cb(m, u);
}
void forEachField(void* k, FieldCb cb, void* u) {
    if (!k || !_cfields) return;
    void* it = nullptr;
    while (void* f = _cfields(k, &it)) cb(f, u);
}
struct InstCtx { void* target; InstCb cb; void* user; };
static void gc_cb(void* obj, void* u) {
    auto* c = (InstCtx*)u;
    if (spinox::il2cpp::objectGetClass(obj) == c->target) c->cb(obj, c->user);
}
void forEachInstance(void* k, InstCb cb, void* u) {
    if (!_gcfe || !k || !cb) return;
    InstCtx c{k, cb, u}; _gcfe((void*)gc_cb, &c);
}
const char* methodName(void* m) { return (_mname && m) ? _mname(m) : ""; }
int methodArgc(void* m) { return (_mpc && m) ? _mpc(m) : 0; }
void* methodReturnType(void* m) { return (_mret && m) ? _mret(m) : nullptr; }
void* methodParamType(void* m, int i) { return (_mpt && m) ? _mpt(m, i) : nullptr; }
uint32_t methodRva(void* m) { return (_mrva && m) ? _mrva(m) : 0; }
void* runtimeInvoke(void* m, void* obj, void** args, void** exc) {
    if (!_invoke || !m) return nullptr;
    return _invoke(m, obj, args, exc);
}
const char* fieldName(void* f) { return (_fname && f) ? _fname(f) : ""; }
void* fieldType(void* f) { return (_ftype && f) ? _ftype(f) : nullptr; }
size_t fieldOffset(void* f) { return (_foff && f) ? _foff(f) : 0; }
bool fieldIsStatic(void* f) { return (_fstat && f) ? _fstat(f) : false; }
void fieldGet(void* o, void* f, void* out) { if (_fget && f) _fget(o, f, out); }
void fieldSet(void* o, void* f, const void* in) { if (_fset && f) _fset(o, f, in); }
void fieldStaticGet(void* f, void* out) { if (_fgets && f) _fgets(f, out); }
void fieldStaticSet(void* f, const void* in) { if (_fsets && f) _fsets(f, in); }
char* typeNameAlloc(void* t) { return (_tname && t) ? _tname(t) : nullptr; }
void freeString(char* s) { if (_free && s) _free(s); }
int typeKind(void* t) { return (_tenum && t) ? _tenum(t) : -1; }
void* typeClass(void* t) { return (_tclass && t) ? _tclass(t) : nullptr; }
void* objectGetClass(void* o) { return (_ogetc && o) ? _ogetc(o) : nullptr; }
std::string readIl2cppString(void* s) {
    if (!s || !_strlen || !_strch) return "";
    int32_t n = _strlen(s);
    if (n <= 0 || n > 4096) return "";
    uint16_t* c = _strch(s); if (!c) return "";
    std::string r; r.reserve(n);
    for (int32_t i = 0; i < n; i++) r.push_back(c[i] < 0x80 ? (char)c[i] : '?');
    return r;
}
void* makeIl2cppString(const char* s) { return (_strnew && s) ? _strnew(s) : nullptr; }
}

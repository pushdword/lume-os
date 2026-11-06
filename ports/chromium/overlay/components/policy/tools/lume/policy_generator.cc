#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#else
#include <dlfcn.h>
#endif

namespace
{

namespace fs = std::filesystem;
using NativeString = fs::path::string_type;
struct PythonObject;
using PythonSize = std::intptr_t;
using PythonRef = std::unique_ptr<PythonObject, void (*)(PythonObject *)>;

void *FindSymbol(void *library, const char *name)
{
#if defined(_WIN32)
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
}

template <typename Function> bool BindSymbol(void *library, const char *name, Function &function)
{
    void *symbol = FindSymbol(library, name);
    if (!symbol)
    {
        std::cerr << "Missing Python API: " << name << '\n';
        return false;
    }
    static_assert(sizeof(function) == sizeof(symbol));
    std::memcpy(&function, &symbol, sizeof(function));
    return true;
}

struct PythonAPI
{
    void (*Initialize)(int) = nullptr;
    int (*IsInitialized)() = nullptr;
    int (*Finalize)() = nullptr;
    void (*SetHome)(const wchar_t *) = nullptr;
    void (*SetProgram)(const wchar_t *) = nullptr;
    wchar_t *(*DecodeLocale)(const char *, std::size_t *) = nullptr;
    void (*FreeRaw)(void *) = nullptr;
    PythonObject *(*UnicodeWide)(const wchar_t *, PythonSize) = nullptr;
    PythonObject *(*UnicodeFS)(const char *) = nullptr;
    PythonObject *(*UnicodeUTF8)(const char *) = nullptr;
    PythonObject *(*SysGet)(const char *) = nullptr;
    int (*SysSet)(const char *, PythonObject *) = nullptr;
    PythonObject *(*ListNew)(PythonSize) = nullptr;
    PythonSize (*ListSize)(PythonObject *) = nullptr;
    int (*ListAppend)(PythonObject *, PythonObject *) = nullptr;
    int (*ListInsert)(PythonObject *, PythonSize, PythonObject *) = nullptr;
    int (*Contains)(PythonObject *, PythonObject *) = nullptr;
    PythonObject *(*Import)(const char *) = nullptr;
    PythonObject *(*GetAttribute)(PythonObject *, const char *) = nullptr;
    PythonObject *(*DictGet)(PythonObject *, const char *) = nullptr;
    PythonObject *(*Call)(PythonObject *, PythonObject *) = nullptr;
    int (*IsTrue)(PythonObject *) = nullptr;
    PythonObject *(*Error)() = nullptr;
    void (*PrintError)(int) = nullptr;
    void (*Decrement)(PythonObject *) = nullptr;

    bool Load(void *library)
    {
        return BindSymbol(library, "Py_InitializeEx", Initialize) &&
               BindSymbol(library, "Py_IsInitialized", IsInitialized) &&
               BindSymbol(library, "Py_FinalizeEx", Finalize) &&
               BindSymbol(library, "Py_SetPythonHome", SetHome) &&
               BindSymbol(library, "Py_SetProgramName", SetProgram) &&
               BindSymbol(library, "Py_DecodeLocale", DecodeLocale) &&
               BindSymbol(library, "PyMem_RawFree", FreeRaw) &&
               BindSymbol(library, "PyUnicode_FromWideChar", UnicodeWide) &&
               BindSymbol(library, "PyUnicode_DecodeFSDefault", UnicodeFS) &&
               BindSymbol(library, "PyUnicode_FromString", UnicodeUTF8) &&
               BindSymbol(library, "PySys_GetObject", SysGet) &&
               BindSymbol(library, "PySys_SetObject", SysSet) &&
               BindSymbol(library, "PyList_New", ListNew) &&
               BindSymbol(library, "PyList_Size", ListSize) &&
               BindSymbol(library, "PyList_Append", ListAppend) &&
               BindSymbol(library, "PyList_Insert", ListInsert) &&
               BindSymbol(library, "PySequence_Contains", Contains) &&
               BindSymbol(library, "PyImport_ImportModule", Import) &&
               BindSymbol(library, "PyObject_GetAttrString", GetAttribute) &&
               BindSymbol(library, "PyDict_GetItemString", DictGet) &&
               BindSymbol(library, "PyObject_CallObject", Call) &&
               BindSymbol(library, "PyObject_IsTrue", IsTrue) &&
               BindSymbol(library, "PyErr_Occurred", Error) &&
               BindSymbol(library, "PyErr_PrintEx", PrintError) &&
               BindSymbol(library, "Py_DecRef", Decrement);
    }

    PythonObject *String(const NativeString &text) const
    {
#if defined(_WIN32)
        return UnicodeWide(text.c_str(), static_cast<PythonSize>(text.size()));
#else
        return UnicodeFS(text.c_str());
#endif
    }
};

fs::path FindRuntime(const fs::path &bin)
{
    std::vector<fs::path> candidates;
    for (const auto &directory : {bin, bin.parent_path() / "lib"})
    {
        std::error_code error;
        fs::directory_iterator it(directory, error), end;
        while (!error && it != end)
        {
            const std::string name = it->path().filename().string();
#if defined(_WIN32)
            const bool matches =
                name.starts_with("python3") && name.size() > 11 && name.ends_with(".dll");
#else
            const bool matches =
                name.starts_with("libpython3") &&
                (name.find(".so") != std::string::npos || name.ends_with(".dylib"));
#endif
            if (matches)
                candidates.push_back(it->path());
            it.increment(error);
        }
    }
    std::sort(candidates.begin(), candidates.end());
    return candidates.empty() ? fs::path() : candidates.front();
}

bool SetRuntimeFlags(void *library)
{
    for (const char *name : {"Py_IsolatedFlag", "Py_IgnoreEnvironmentFlag", "Py_NoSiteFlag",
                             "Py_DontWriteBytecodeFlag"})
    {
        auto *flag = static_cast<int *>(FindSymbol(library, name));
        if (!flag)
        {
            std::cerr << "Missing Python runtime flag: " << name << '\n';
            return false;
        }
        *flag = 1;
    }
    return true;
}

int Generate(PythonAPI &python, const std::vector<NativeString> &arguments)
{
    const fs::path generator = fs::absolute(fs::path(arguments[2]));
    PythonRef argv(python.ListNew(0), python.Decrement);
    if (!argv)
        return 1;
    for (std::size_t index = 2; index < arguments.size(); ++index)
    {
        PythonRef value(python.String(arguments[index]), python.Decrement);
        if (!value || python.ListAppend(argv.get(), value.get()) != 0)
            return 1;
    }
    if (python.SysSet("argv", argv.get()) != 0)
        return 1;

    PythonObject *path = python.SysGet("path");
    PythonRef directory(python.String(generator.parent_path().native()), python.Decrement);
    if (!path || !directory || python.ListInsert(path, 0, directory.get()) != 0)
        return 1;

    const std::string module_name = generator.stem().string();
    PythonRef module(python.Import(module_name.c_str()), python.Decrement);
    if (!module)
        return 1;
    PythonRef platforms(python.GetAttribute(module.get(), "PLATFORM_STRINGS"), python.Decrement);
    if (!platforms)
        return 1;
    PythonObject *desktop = python.DictGet(platforms.get(), "chrome.*");
    if (!desktop || python.ListSize(desktop) < 0)
        return 1;
    PythonRef native(python.UnicodeUTF8("lume"), python.Decrement);
    if (!native)
        return 1;
    const int contains = python.Contains(desktop, native.get());
    if (contains < 0 || (contains == 0 && python.ListAppend(desktop, native.get()) != 0))
        return 1;

    PythonRef entry(python.GetAttribute(module.get(), "main"), python.Decrement);
    if (!entry)
        return 1;
    PythonRef result(python.Call(entry.get(), nullptr), python.Decrement);
    if (!result)
        return 1;
    const int status = python.IsTrue(result.get());
    return status == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char **argv)
{
    std::vector<NativeString> arguments;
#if defined(_WIN32)
    int count = 0;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!wide)
        return 1;
    for (int index = 0; index < count; ++index)
        arguments.emplace_back(wide[index]);
    LocalFree(wide);
#else
    for (int index = 0; index < argc; ++index)
        arguments.emplace_back(argv[index]);
#endif
    if (arguments.size() < 3)
    {
        std::cerr << "Usage: lume_policy_generator PYTHON_BIN GENERATOR [arguments]\n";
        return 1;
    }
    const fs::path bin = fs::absolute(fs::path(arguments[1]));
    const fs::path runtime = FindRuntime(bin);
    if (runtime.empty())
    {
        std::cerr << "Python shared runtime not found\n";
        return 1;
    }
#if defined(_WIN32)
    void *library =
        LoadLibraryExW(runtime.c_str(), nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
    void *library = dlopen(runtime.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (!library)
    {
        std::cerr << "Failed to load Python shared runtime\n";
        return 1;
    }
    PythonAPI python;
    int result = 1;
    if (python.Load(library) && SetRuntimeFlags(library))
    {
        const fs::path home = fs::is_directory(bin / "Lib") ? bin : bin.parent_path();
#if defined(_WIN32)
        const std::wstring home_string = home.native();
        const std::wstring program_string = (bin / "python3.exe").native();
        python.SetHome(home_string.c_str());
        python.SetProgram(program_string.c_str());
#else
        wchar_t *home_string = python.DecodeLocale(home.c_str(), nullptr);
        wchar_t *program_string = python.DecodeLocale((bin / "python3").c_str(), nullptr);
        if (home_string && program_string)
        {
            python.SetHome(home_string);
            python.SetProgram(program_string);
        }
#endif
#if defined(_WIN32)
        const bool configuration_ready = true;
#else
        const bool configuration_ready = home_string && program_string;
#endif
        if (configuration_ready)
            python.Initialize(0);
        if (configuration_ready && python.IsInitialized())
        {
            result = Generate(python, arguments);
            if (python.Error())
                python.PrintError(0);
            if (python.Finalize() < 0)
                result = 1;
        }
#if !defined(_WIN32)
        python.FreeRaw(home_string);
        python.FreeRaw(program_string);
#endif
    }
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(library));
#else
    dlclose(library);
#endif
    return result;
}

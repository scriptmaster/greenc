#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <new>
#include <string>

static HMODULE module;
static LONG objects;
static const CLSID command_id = {0x8ea263cc,0xf47a,0x40a0,{0x8e,0xdb,0x24,0x8a,0x64,0xdd,0xa2,0x31}};

static std::wstring editor_path() {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    if (!length || length >= MAX_PATH) { return {}; }

    PathRemoveFileSpecW(path);
    return std::wstring(path) + L"\\editor.exe";
}

class Command final : public IExplorerCommand, public IObjectWithSite {
    LONG refs = 1;
    IUnknown* site = nullptr;
public:
    Command() { InterlockedIncrement(&objects); }
    ~Command() { if (site) { site->Release(); } InterlockedDecrement(&objects); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override {
        *out = nullptr;
        if (id == IID_IUnknown || id == IID_IExplorerCommand) { *out = static_cast<IExplorerCommand*>(this); }
        else if (id == IID_IObjectWithSite) { *out = static_cast<IObjectWithSite*>(this); }
        else { return E_NOINTERFACE; }

        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG left = InterlockedDecrement(&refs);
        if (!left) { delete this; }
        return left;
    }
    HRESULT STDMETHODCALLTYPE SetSite(IUnknown* next) override {
        if (next) { next->AddRef(); }
        if (site) { site->Release(); }
        site = next;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetSite(REFIID id, void** out) override {
        return site ? site->QueryInterface(id, out) : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE GetTitle(IShellItemArray*, LPWSTR* out) override { return SHStrDupW(L"Open in GreenC Editor", out); }
    HRESULT STDMETHODCALLTYPE GetIcon(IShellItemArray*, LPWSTR* out) override { return SHStrDupW((editor_path()+L",0").c_str(), out); }
    HRESULT STDMETHODCALLTYPE GetToolTip(IShellItemArray*, LPWSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetCanonicalName(GUID* out) override { *out = command_id; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetState(IShellItemArray*, BOOL, EXPCMDSTATE* out) override { *out = ECS_ENABLED; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetFlags(EXPCMDFLAGS* out) override { *out = ECF_DEFAULT; return S_OK; }
    HRESULT STDMETHODCALLTYPE EnumSubCommands(IEnumExplorerCommand**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Invoke(IShellItemArray* selection, IBindCtx*) override {
        IShellItem* item = nullptr;
        if (selection) { selection->GetItemAt(0, &item); }

        if (!item && site) {
            IFolderView* view = nullptr;
            if (SUCCEEDED(IUnknown_QueryService(site, SID_SFolderView, IID_PPV_ARGS(&view)))) {
                IPersistFolder2* folder = nullptr;
                if (SUCCEEDED(view->GetFolder(IID_PPV_ARGS(&folder)))) {
                    PIDLIST_ABSOLUTE pidl = nullptr;
                    if (SUCCEEDED(folder->GetCurFolder(&pidl))) {
                        SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&item));
                        CoTaskMemFree(pidl);
                    }
                    folder->Release();
                }
                view->Release();
            }
        }

        if (!item) { return E_FAIL; }
        PWSTR path = nullptr;
        HRESULT result = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
        item->Release();
        if (FAILED(result)) { return result; }

        std::wstring args = L"--folder \"";
        args += path;
        if (args.back() == L'\\') { args += L'\\'; }
        args += L'"';
        CoTaskMemFree(path);
        SHELLEXECUTEINFOW launch = {sizeof(launch)};
        std::wstring exe = editor_path();
        launch.lpFile = exe.c_str();
        launch.lpParameters = args.c_str();
        launch.nShow = SW_SHOWNORMAL;
        return ShellExecuteExW(&launch) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
};

class Factory final : public IClassFactory {
    LONG refs = 1;
public:
    Factory() { InterlockedIncrement(&objects); }
    ~Factory() { InterlockedDecrement(&objects); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override {
        *out = nullptr;
        if (id != IID_IUnknown && id != IID_IClassFactory) { return E_NOINTERFACE; }
        *out = this;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG left = InterlockedDecrement(&refs);
        if (!left) { delete this; }
        return left;
    }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID id, void** out) override {
        if (outer) { return CLASS_E_NOAGGREGATION; }
        auto command = new(std::nothrow) Command;
        if (!command) { return E_OUTOFMEMORY; }
        HRESULT result = command->QueryInterface(id, out);
        command->Release();
        return result;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
        if (lock) { InterlockedIncrement(&objects); } else { InterlockedDecrement(&objects); }
        return S_OK;
    }
};

extern "C" __declspec(dllexport) HRESULT WINAPI DllCanUnloadNow() { return objects ? S_FALSE : S_OK; }
extern "C" __declspec(dllexport) HRESULT WINAPI DllGetClassObject(REFCLSID cls, REFIID id, void** out) {
    if (cls != command_id) { return CLASS_E_CLASSNOTAVAILABLE; }
    auto factory = new(std::nothrow) Factory;
    if (!factory) { return E_OUTOFMEMORY; }
    HRESULT result = factory->QueryInterface(id, out);
    factory->Release();
    return result;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { module = instance; }
    return TRUE;
}

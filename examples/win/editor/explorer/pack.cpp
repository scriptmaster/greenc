// Build the sparse identity package with the Windows AppxPackaging COM API.
// The editor and Explorer DLL remain together in the external bin directory.
#include <windows.h>
#include <shlwapi.h>
#include <urlmon.h>
#include <cstdio>

struct PackageSettings { BOOL zip64; IUri* hash_method; };
struct PackageWriter : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE AddPayloadFile(LPCWSTR, LPCWSTR, int, IStream*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(IStream*) = 0;
};
struct PackageFactory : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreatePackageWriter(IStream*, PackageSettings*, PackageWriter**) = 0;
};

int wmain(int argc, wchar_t** argv) {
    if (argc != 4) { return 2; }
    const CLSID factory_class = {0x5842a140,0xff9f,0x4166,{0x8f,0x5c,0x62,0xf5,0xb7,0xb0,0xc7,0x81}};
    const IID factory_interface = {0xbeb94909,0xe451,0x438b,{0xb5,0xa7,0xd7,0x9e,0x76,0x7b,0x75,0xd8}};
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) { return 1; }

    PackageFactory* factory = nullptr;
    PackageWriter* writer = nullptr;
    IStream *manifest = nullptr, *logo = nullptr, *output = nullptr;
    PackageSettings settings = {};
    hr = CoCreateInstance(factory_class, nullptr, CLSCTX_INPROC_SERVER, factory_interface, reinterpret_cast<void**>(&factory));
    if (SUCCEEDED(hr)) { hr = SHCreateStreamOnFileEx(argv[1], STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, nullptr, &manifest); }
    if (SUCCEEDED(hr)) { hr = SHCreateStreamOnFileEx(argv[2], STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, nullptr, &logo); }
    if (SUCCEEDED(hr)) { hr = SHCreateStreamOnFileEx(argv[3], STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, TRUE, nullptr, &output); }
    if (SUCCEEDED(hr)) { hr = CreateUri(L"http://www.w3.org/2001/04/xmlenc#sha256", 0, 0, &settings.hash_method); }
    if (SUCCEEDED(hr)) { hr = factory->CreatePackageWriter(output, &settings, &writer); }
    if (SUCCEEDED(hr)) { hr = writer->AddPayloadFile(L"logo.png", L"image/png", 0, logo); }
    if (SUCCEEDED(hr)) { hr = writer->Close(manifest); }

    if (writer) { writer->Release(); }
    if (settings.hash_method) { settings.hash_method->Release(); }
    if (output) { output->Release(); }
    if (logo) { logo->Release(); }
    if (manifest) { manifest->Release(); }
    if (factory) { factory->Release(); }
    CoUninitialize();

    if (FAILED(hr)) { std::fprintf(stderr, "Explorer MSIX packaging failed (0x%08lx)\n", static_cast<unsigned long>(hr)); }
    return FAILED(hr) ? 1 : 0;
}

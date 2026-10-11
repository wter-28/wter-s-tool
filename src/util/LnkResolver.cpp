#include "LnkResolver.h"

#ifdef Q_OS_WIN
#include <QDir>
#include <objbase.h>
#include <shlobj.h>
#include <windows.h>

namespace {

// 手工声明 GUID，避免依赖 MinGW 的 IID_* 宏
const GUID kCLSID_ShellLink =
    {0x00021401, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID kIID_IShellLinkW =
    {0x000214F9, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID kIID_IPersistFile =
    {0x0000010B, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

} // namespace

namespace LnkResolver {

QString resolve(const QString &lnkPath)
{
    QString result;
    if (!lnkPath.endsWith(QStringLiteral(".lnk"), Qt::CaseInsensitive))
        return lnkPath;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE)
        hr = S_OK; // 已在本线程初始化，继续使用

    IShellLinkW *link = nullptr;
    hr = CoCreateInstance(kCLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                          kIID_IShellLinkW, reinterpret_cast<void **>(&link));
    if (SUCCEEDED(hr) && link) {
        IPersistFile *pf = nullptr;
        hr = link->QueryInterface(kIID_IPersistFile, reinterpret_cast<void **>(&pf));
        if (SUCCEEDED(hr) && pf) {
            hr = pf->Load(reinterpret_cast<const wchar_t *>(lnkPath.utf16()), STGM_READ);
            if (SUCCEEDED(hr)) {
                wchar_t buf[MAX_PATH] = {0};
                if (SUCCEEDED(link->GetPath(buf, MAX_PATH, nullptr, SLGP_UNCPRIORITY)) && buf[0])
                    result = QDir::fromNativeSeparators(QString::fromWCharArray(buf));
            }
            pf->Release();
        }
        link->Release();
    }

    if (needUninit)
        CoUninitialize();

    return result;
}

} // namespace LnkResolver

#else

namespace LnkResolver {
QString resolve(const QString &lnkPath) { return lnkPath; }
} // namespace LnkResolver

#endif

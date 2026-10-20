#include <windows.h>
#include <shlobj.h>
#include <string>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

// UI Kontrolleri için ID'ler
#define ID_BTN_BROWSE  101
#define ID_BTN_RENAME  102
#define ID_EDIT_PATH   103
#define ID_EDIT_PREFIX 104
#define ID_COMBO_LANG  105
#define ID_LABEL_PATH  106
#define ID_LABEL_PREFIX 107
#define ID_LABEL_LANG  108

// Dil Sıralaması
enum Language { LANG_EN = 0, LANG_TR = 1, LANG_DE = 2, LANG_FR = 3 };
int currentLang = LANG_EN; // Varsayılan İngilizce

struct LocalizedText {
    const wchar_t* title;
    const wchar_t* btnBrowse;
    const wchar_t* btnRename;
    const wchar_t* lblPath;
    const wchar_t* lblPrefix;
    const wchar_t* lblLang;
    const wchar_t* successMsg;
    const wchar_t* errorMsg;
};

LocalizedText texts[] = {
    { L"Batch File Renamer", L"Browse...", L"Rename Files", L"Select Folder:", L"Prefix (e.g. Vacation_):", L"Language / Dil:", L"Process completed successfully!", L"Please select a folder and enter a prefix!" },
    { L"Toplu Dosya Düzenleyici", L"Gözat...", L"Dosyaları Yeniden Adlandır", L"Klasör Seçin:", L"Yeni Önek (Örn: Tatil_):", L"Dil / Language:", L"İşlem başarıyla tamamlandı!", L"Lütfen klasör seçin ve önek girin!" },
    { L"Batch Datei-Umbenenner", L"Durchsuchen...", L"Dateien Umbenennen", L"Ordner Wählen:", L"Präfix (z.B. Urlaub_):", L"Sprache:", L"Vorgang erfolgreich abgeschlossen!", L"Bitte wählen Sie einen Ordner und geben Sie ein Präfix ein!" },
    { L"Renommeur de Fichiers", L"Parcourir...", L"Renommer les Fichiers", L"Sélectionner le Dossier:", L"Préfixe (ex: Vacances_):", L"Langue:", L"Opération terminée avec succès!", L"Veuillez sélectionner un dossier et entrer un préfixe!" }
};

// Global Kontroller ve Modern Tasarım Unsurları
HWND hEditPath, hEditPrefix, hComboLang;
HWND hBtnBrowse, hBtnRename, hLblPath, hLblPrefix, hLblLang;
std::wstring selectedFolderPath = L"";

HBRUSH hBrushDarkBg = CreateSolidBrush(RGB(32, 32, 32));     // Modern Koyu Gri Arka Plan
HBRUSH hBrushControl = CreateSolidBrush(RGB(45, 45, 45));    // Kutu Arka Planı
HFONT hModernFont = NULL;                                    // Segoe UI Modern Fontu

// Metinleri Güncelleme Fonksiyonu
void UpdateLanguage(HWND hwnd) {
    SetWindowTextW(hwnd, texts[currentLang].title);
    SetWindowTextW(hBtnBrowse, texts[currentLang].btnBrowse);
    SetWindowTextW(hBtnRename, texts[currentLang].btnRename);
    SetWindowTextW(hLblPath, texts[currentLang].lblPath);
    SetWindowTextW(hLblPrefix, texts[currentLang].lblPrefix);
    SetWindowTextW(hLblLang, texts[currentLang].lblLang);
}

// Klasör Seçme Penceresi
std::wstring SelectFolder(HWND hwnd) {
    WCHAR path[MAX_PATH] = { 0 };
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Select a folder / Klasör seçin";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        if (SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            return std::wstring(path);
        }
        CoTaskMemFree(pidl);
    }
    return L"";
}

// Yeniden Adlandırma Mantığı (Win32 API Tabanlı - Hatasız)
void RenameFiles(HWND hwnd) {
    WCHAR prefix[256];
    GetWindowTextW(hEditPrefix, prefix, 256);

    if (selectedFolderPath.empty() || wcslen(prefix) == 0) {
        MessageBoxW(hwnd, texts[currentLang].errorMsg, L"Warning / Uyarı", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring searchPath = selectedFolderPath + L"\\*.*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBoxW(hwnd, L"Folder could not be opened!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    int counter = 1;
    do {
        // Sadece dosyaları al, klasörleri ve sistem dizinlerini atla
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring oldFileName = findData.cFileName;

            // Uzantıyı bul (.png, .jpg vb.)
            size_t dotPos = oldFileName.find_last_of(L".");
            std::wstring ext = (dotPos != std::wstring::npos) ? oldFileName.substr(dotPos) : L"";

            std::wstring oldFullPath = selectedFolderPath + L"\\" + oldFileName;
            std::wstring newFileName = prefix + std::to_wstring(counter) + ext;
            std::wstring newFullPath = selectedFolderPath + L"\\" + newFileName;

            _wrename(oldFullPath.c_str(), newFullPath.c_str());
            counter++;
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
    MessageBoxW(hwnd, texts[currentLang].successMsg, L"Info / Bilgi", MB_OK | MB_ICONINFORMATION);
}

// Pencere Olayları ve Modern Görünüm Yönetimi
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        hModernFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        hLblPath = CreateWindowW(L"STATIC", texts[0].lblPath, WS_VISIBLE | WS_CHILD, 20, 20, 300, 20, hwnd, (HMENU)ID_LABEL_PATH, NULL, NULL);
        hEditPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY, 20, 45, 340, 28, hwnd, (HMENU)ID_EDIT_PATH, NULL, NULL);
        hBtnBrowse = CreateWindowW(L"BUTTON", texts[0].btnBrowse, WS_VISIBLE | WS_CHILD, 370, 45, 95, 28, hwnd, (HMENU)ID_BTN_BROWSE, NULL, NULL);

        hLblPrefix = CreateWindowW(L"STATIC", texts[0].lblPrefix, WS_VISIBLE | WS_CHILD, 20, 88, 300, 20, hwnd, (HMENU)ID_LABEL_PREFIX, NULL, NULL);
        hEditPrefix = CreateWindowW(L"EDIT", L"File_", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 113, 445, 28, hwnd, (HMENU)ID_EDIT_PREFIX, NULL, NULL);

        hLblLang = CreateWindowW(L"STATIC", texts[0].lblLang, WS_VISIBLE | WS_CHILD, 20, 158, 130, 20, hwnd, (HMENU)ID_LABEL_LANG, NULL, NULL);
        hComboLang = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 160, 155, 160, 120, hwnd, (HMENU)ID_COMBO_LANG, NULL, NULL);

        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Türkçe");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Deutsch");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Français");
        SendMessageW(hComboLang, CB_SETCURSEL, 0, 0);

        hBtnRename = CreateWindowW(L"BUTTON", texts[0].btnRename, WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 205, 445, 42, hwnd, (HMENU)ID_BTN_RENAME, NULL, NULL);

        HWND controls[] = { hLblPath, hEditPath, hBtnBrowse, hLblPrefix, hEditPrefix, hLblLang, hComboLang, hBtnRename };
        for (HWND hCtrl : controls) {
            SendMessageW(hCtrl, WM_SETFONT, (WPARAM)hModernFont, TRUE);
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wp;
        SetTextColor(hdcStatic, RGB(230, 230, 230));
        SetBkColor(hdcStatic, RGB(32, 32, 32));
        return (INT_PTR)hBrushDarkBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wp;
        SetTextColor(hdcEdit, RGB(255, 255, 255));
        SetBkColor(hdcEdit, RGB(45, 45, 45));
        return (INT_PTR)hBrushControl;
    }

    case WM_CTLCOLORBTN:
        return (INT_PTR)hBrushDarkBg;

    case WM_CTLCOLORLISTBOX: {
        HDC hdcListbox = (HDC)wp;
        SetTextColor(hdcListbox, RGB(255, 255, 255));
        SetBkColor(hdcListbox, RGB(45, 45, 45));
        return (INT_PTR)hBrushControl;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == ID_BTN_BROWSE) {
            std::wstring path = SelectFolder(hwnd);
            if (!path.empty()) {
                selectedFolderPath = path;
                SetWindowTextW(hEditPath, selectedFolderPath.c_str());
            }
        }
        else if (LOWORD(wp) == ID_BTN_RENAME) {
            RenameFiles(hwnd);
        }
        else if (HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == ID_COMBO_LANG) {
            int sel = (int)SendMessageW(hComboLang, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel <= 3) {
                currentLang = sel;
                UpdateLanguage(hwnd);
            }
        }
        break;

    case WM_DESTROY:
        if (hModernFont) DeleteObject(hModernFont);
        DeleteObject(hBrushDarkBg);
        DeleteObject(hBrushControl);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = hBrushDarkBg;
    wc.lpszClassName = L"ModernBatchRenamerClass";

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"ModernBatchRenamerClass", L"Batch File Renamer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 505, 305, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (0);
}
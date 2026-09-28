// MenuPage.cpp

#include "StdAfx.h"

#include "../Common/ZipRegistry.h"

#include "../../../Windows/DLL.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileFind.h"

#include "../Explorer/ContextMenuFlags.h"
#include "../Explorer/RegistryContextMenu.h"
#include "../Explorer/resource.h"

#include "../FileManager/PropertyNameRes.h"

#include "../GUI/ExtractDialogRes.h"

#include "FormatUtils.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "MenuPage.h"
#include "MenuPageRes.h"
#include "ShellIntegrationModern.h"

#ifdef ZIP7_DARKMODE
#include "../../../../DarkMode/lib/include/Darkmodelib.h"
#endif

using namespace NWindows;
using namespace NContextMenuFlags;

#ifdef Z7_LANG
static const UInt32 kLangIDs[] =
{
  IDX_SYSTEM_INTEGRATE_TO_MENU,
  IDX_SYSTEM_CASCADED_MENU,
  IDX_SYSTEM_ICON_IN_MENU,
  IDX_EXTRACT_ELIM_DUP,
  IDT_SYSTEM_ZONE,
  IDT_SYSTEM_CONTEXT_MENU_ITEMS
};
#endif

#define kMenuTopic "fm/options.htm#sevenZip"

struct CContextMenuItem
{
  unsigned ControlID;
  UInt32 Flag;
};

static const CContextMenuItem kMenuItems[] =
{
  { IDS_CONTEXT_OPEN, kOpen },
  { IDS_CONTEXT_OPEN, kOpenAs },
  { IDS_CONTEXT_EXTRACT, kExtract },
  { IDS_CONTEXT_EXTRACT_HERE, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, kExtractTo },

  { IDS_CONTEXT_TEST, kTest },

  { IDS_CONTEXT_COMPRESS, kCompress },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO, kCompressToZip },

  #ifndef UNDER_CE
  { IDS_CONTEXT_COMPRESS_EMAIL, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressToZipEmail },
  #endif

  { IDS_PROP_CHECKSUM, kCRC },
  { IDS_PROP_CHECKSUM, kCRC_Cascaded },
};


#if !defined(_WIN64)
extern bool g_Is_Wow64;
#endif

#ifndef KEY_WOW64_64KEY
  #define KEY_WOW64_64KEY (0x0100)
#endif

#ifndef KEY_WOW64_32KEY
  #define KEY_WOW64_32KEY (0x0200)
#endif


static void LoadLang_Spec(UString &s, UInt32 id, const char *eng)
{
  LangString(id, s);
  if (s.IsEmpty())
    s = eng;
  s.RemoveChar(L'&');
}


/* The texts of the new mode controls. A language file may provide them under
   the same ids; the built-in fallback is Chinese (written with \u escapes so
   that this source file stays pure ASCII - MSVC reads it as codepage 936 and
   -WX turns warning C4819 into an error otherwise). */
static void Set_ModeControl_Text(HWND hwnd, unsigned id, const wchar_t *fallback)
{
  UString s;
  LangString(id, s);
  if (s.IsEmpty())
    s = fallback;
  ::SetDlgItemTextW(hwnd, id, s.Ptr());
}

static void Set_ModeControls_Text(HWND hwnd)
{
  Set_ModeControl_Text(hwnd, IDT_SYSTEM_MENU_MODE,
      L"\u53F3\u952E\u83DC\u5355\u96C6\u6210\uFF1A");   // "Context menu integration:"
  Set_ModeControl_Text(hwnd, IDX_SYSTEM_MENU_CLASSIC,
      L"\u7ECF\u5178\u83DC\u5355\uFF08\u201C\u663E\u793A\u66F4\u591A\u9009\u9879\u201D\uFF09");
  Set_ModeControl_Text(hwnd, IDX_SYSTEM_MENU_MODERN,
      L"Windows 11 \u65B0\u83DC\u5355\uFF08\u7A00\u758F\u5305\uFF09");
  Set_ModeControl_Text(hwnd, IDX_SYSTEM_MENU_BOTH,
      L"\u4E24\u8005\u90FD\u6CE8\u518C\uFF08\u6587\u4EF6\u4E0A\u4F1A\u91CD\u590D\uFF09");
  Set_ModeControl_Text(hwnd, IDX_SYSTEM_MENU_NONE,
      L"\u90FD\u4E0D\u6CE8\u518C");
}


bool CMenuPage::OnInit()
{
  _initMode = true;

  Clear_MenuChanged();
  
#ifdef Z7_LANG
  LangSetDlgItems(*this, kLangIDs, Z7_ARRAY_SIZE(kLangIDs));
#endif

  #ifdef UNDER_CE

  HideItem(IDX_SYSTEM_INTEGRATE_TO_MENU);
  HideItem(IDX_SYSTEM_INTEGRATE_TO_MENU_2);

  #else

  {
    UString s;
    {
      CWindow window(GetItem(IDX_SYSTEM_INTEGRATE_TO_MENU));
      window.GetText(s);
    }
    UString bit64 = LangString(IDS_PROP_BIT64);
    if (bit64.IsEmpty())
      bit64 = "64-bit";
    #ifdef _WIN64
      bit64.Replace(L"64", L"32");
    #endif
    s.Add_Space();
    s.Add_Char('(');
    s += bit64;
    s.Add_Char(')');
    SetItemText(IDX_SYSTEM_INTEGRATE_TO_MENU_2, s);
  }

  const FString prefix = NDLL::GetModuleDirPrefix();
  
  _dlls[0].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU;
  _dlls[1].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU_2;
  
  _dlls[0].wow = 0;
  _dlls[1].wow =
      #ifdef _WIN64
        KEY_WOW64_32KEY
      #else
        KEY_WOW64_64KEY
      #endif
      ;

  for (unsigned d = 0; d < 2; d++)
  {
    CShellDll &dll = _dlls[d];

    dll.wasChanged = false;

    #ifndef _WIN64
    if (d != 0 && !g_Is_Wow64)
    {
      HideItem(dll.ctrl);
      continue;
    }
    #endif

    FString &path = dll.Path;
    path = prefix;
    path += (d == 0 ? "7-zip.dll" :
        #ifdef _WIN64
          "7-zip32.dll"
        #else
          "7-zip64.dll"
        #endif
        );


    if (!NFile::NFind::DoesFileExist_Raw(path))
    {
      path.Empty();
      EnableItem(dll.ctrl, false);
    }
    else
    {
      dll.prevValue = CheckContextMenuHandler(fs2us(path), dll.wow);
      CheckButton(dll.ctrl, dll.prevValue);
    }
  }

  #endif


  Set_ModeControls_Text(*this);
  Update_MenuMode_Controls();

  CContextMenuInfo ci;
  ci.Load();

  CheckButton(IDX_SYSTEM_CASCADED_MENU, ci.Cascaded.Val);
  CheckButton(IDX_SYSTEM_ICON_IN_MENU, ci.MenuIcons.Val);
  CheckButton(IDX_EXTRACT_ELIM_DUP, ci.ElimDup.Val);

  _listView.Attach(GetItem(IDL_SYSTEM_OPTIONS));
  _zoneCombo.Attach(GetItem(IDC_SYSTEM_ZONE));

  {
    unsigned wz = ci.WriteZone;
    if (wz == (UInt32)(Int32)-1)
      wz = 0;
    for (unsigned i = 0; i <= 3; i++)
    {
      unsigned val = i;
      UString s;
      if (i == 3)
      {
        if (wz < 3)
          break;
        val = wz;
      }
      else
      {
        #define MY_IDYES  406
        #define MY_IDNO   407
        if (i == 0)
          LoadLang_Spec(s, MY_IDNO, "No");
        else if (i == 1)
          LoadLang_Spec(s, MY_IDYES, "Yes");
        else
          LangString(IDT_ZONE_FOR_OFFICE, s);
      }
      if (s.IsEmpty())
        s.Add_UInt32(val);
      if (i == 0)
        s.Insert(0, L"* ");
      const int index = (int)_zoneCombo.AddString_SetItemData(s, (LPARAM)val);
      if (val == wz)
        _zoneCombo.SetCurSel(index);
    }
  }


  const UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  _listView.InsertColumn(0, L"", 200);

  for (unsigned i = 0; i < Z7_ARRAY_SIZE(kMenuItems); i++)
  {
    const CContextMenuItem &menuItem = kMenuItems[i];

    UString s = LangString(menuItem.ControlID);
    if (menuItem.Flag == kCRC)
      s = "HASH";
    else if (menuItem.Flag == kCRC_Cascaded)
      s = "7-Zip > HASH";
    if (menuItem.Flag == kOpenAs
        || menuItem.Flag == kCRC
        || menuItem.Flag == kCRC_Cascaded)
       s += " >";

    switch (menuItem.ControlID)
    {
      case IDS_CONTEXT_EXTRACT_TO:
      {
        s = MyFormatNew(s, LangString(IDS_CONTEXT_FOLDER));
        break;
      }
      case IDS_CONTEXT_COMPRESS_TO:
      case IDS_CONTEXT_COMPRESS_TO_EMAIL:
      {
        UString s2 = LangString(IDS_CONTEXT_ARCHIVE);
        switch (menuItem.Flag)
        {
          case kCompressTo7z:
          case kCompressTo7zEmail:
            s2 += (".7z");
            break;
          case kCompressToZip:
          case kCompressToZipEmail:
            s2 += (".zip");
            break;
        }
        s = MyFormatNew(s, s2);
        break;
      }
    }

    const int itemIndex = _listView.InsertItem(i, s);
    _listView.SetCheckState((unsigned)itemIndex, ((ci.Flags & menuItem.Flag) != 0));
  }

  _listView.SetColumnWidthAuto(0);
  _initMode = false;

#ifdef ZIP7_DARKMODE
  dmlib::setDarkListViewCheckboxes(_listView);
#endif

  return CPropertyPage::OnInit();
}


#ifndef UNDER_CE

static void ShowMenuErrorMessage(const wchar_t *m, HWND hwnd)
{
  MessageBoxW(hwnd, m, L"7-Zip ZS", MB_ICONERROR);
}

#endif


CMenuPage::enum_MenuMode CMenuPage::Get_Saved_MenuMode() const
{
  bool classicFiles = false;
  #ifndef UNDER_CE
  if (!_dlls[0].Path.IsEmpty())
    classicFiles = CheckContextMenuHandler(fs2us(_dlls[0].Path), _dlls[0].wow);
  #endif

  const bool modern = NShellIntegrationModern::Is_Installed();

  if (modern)
    return classicFiles ? kMenuMode_Both : kMenuMode_Modern;
  return classicFiles ? kMenuMode_Classic : kMenuMode_None;
}


CMenuPage::enum_MenuMode CMenuPage::Get_Checked_MenuMode() const
{
  if (IsButtonCheckedBool(IDX_SYSTEM_MENU_MODERN)) return kMenuMode_Modern;
  if (IsButtonCheckedBool(IDX_SYSTEM_MENU_BOTH))   return kMenuMode_Both;
  if (IsButtonCheckedBool(IDX_SYSTEM_MENU_NONE))   return kMenuMode_None;
  return kMenuMode_Classic;
}


void CMenuPage::Set_MenuMode_Controls(enum_MenuMode mode)
{
  unsigned id = IDX_SYSTEM_MENU_CLASSIC;
  switch (mode)
  {
    case kMenuMode_Classic: id = IDX_SYSTEM_MENU_CLASSIC; break;
    case kMenuMode_Modern:  id = IDX_SYSTEM_MENU_MODERN; break;
    case kMenuMode_Both:    id = IDX_SYSTEM_MENU_BOTH; break;
    case kMenuMode_None:    id = IDX_SYSTEM_MENU_NONE; break;
  }
  ::CheckRadioButton(*this, IDX_SYSTEM_MENU_CLASSIC, IDX_SYSTEM_MENU_NONE, (int)id);
}


void CMenuPage::Update_MenuMode_Controls()
{
  Set_MenuMode_Controls(Get_Saved_MenuMode());
}


void CMenuPage::Apply_MenuMode(enum_MenuMode mode)
{
  #ifndef UNDER_CE
  const bool wantClassic = (mode == kMenuMode_Classic || mode == kMenuMode_Both);
  const bool wantModern  = (mode == kMenuMode_Modern  || mode == kMenuMode_Both);
  UString error;

  // 1) the machine-wide classic registration (the "*" root, as the checkbox does)
  if (!_dlls[0].Path.IsEmpty())
  {
    const UString path = fs2us(_dlls[0].Path);
    if (CheckContextMenuHandler(path, _dlls[0].wow) != wantClassic)
    {
      const LONG res = SetContextMenuHandler(wantClassic, path, _dlls[0].wow);
      if (res != ERROR_SUCCESS)
        ShowMenuErrorMessage(NError::MyFormatMessage(res), *this);
    }
  }

  if (wantModern)
  {
    // The shell lists the commands of a sparse package in the classic menu for
    // files but not for directories, so Folder/Directory are registered per user
    // (HKCU\Software\Classes - no administrator rights needed).
    if (NShellIntegrationModern::Set_FolderRegistration_PerUser(true, error) != S_OK)
      ShowMenuErrorMessage(error, *this);

    if (!NShellIntegrationModern::Is_Installed())
    {
      const UString msixPath = NShellIntegrationModern::Get_DefaultMsixPath();
      if (msixPath.IsEmpty())
      {
        UString m = L"Package file not found:\n";
        m += L"SevenZipZS.ShellExtension_x64.msix\n\n";
        m += L"Run Package\\build-shell-package.ps1 (as administrator) once.";
        ShowMenuErrorMessage(m, *this);
      }
      else
      {
        const UString dir = fs2us(NDLL::GetModuleDirPrefix());
        if (NShellIntegrationModern::Install(msixPath, dir, error) != S_OK)
        {
          if (error.IsEmpty())
            error = L"AddPackage failed";
          ShowMenuErrorMessage(error, *this);
        }
      }
    }
  }
  else
  {
    if (NShellIntegrationModern::Is_Installed())
    {
      if (NShellIntegrationModern::Remove(error) != S_OK)
      {
        if (error.IsEmpty())
          error = L"RemovePackage failed";
        ShowMenuErrorMessage(error, *this);
      }
    }
    NShellIntegrationModern::Set_FolderRegistration_PerUser(false, error);
  }
  #endif

  // reflect what really is registered now
  Update_MenuMode_Controls();

  #ifndef UNDER_CE
  for (unsigned d = 0; d < 2; d++)
  {
    CShellDll &dll = _dlls[d];
    if (!dll.Path.IsEmpty())
    {
      dll.prevValue = CheckContextMenuHandler(fs2us(dll.Path), dll.wow);
      CheckButton(dll.ctrl, dll.prevValue);
      dll.wasChanged = false;
    }
  }
  #endif
}


LONG CMenuPage::OnApply()
{
  #ifndef UNDER_CE

  if (_menuMode_Changed)
  {
    Apply_MenuMode(Get_Checked_MenuMode());
    _menuMode_Changed = false;
  }

  for (unsigned d = 2; d != 0;)
  {
    d--;
    CShellDll &dll = _dlls[d];
    if (dll.wasChanged && !dll.Path.IsEmpty())
    {
      const bool newVal = IsButtonCheckedBool(dll.ctrl);
      const LONG res = SetContextMenuHandler(newVal, fs2us(dll.Path), dll.wow);
      if (res != ERROR_SUCCESS && (dll.prevValue != newVal || newVal))
        ShowMenuErrorMessage(NError::MyFormatMessage(res), *this);
      dll.prevValue = CheckContextMenuHandler(fs2us(dll.Path), dll.wow);
      CheckButton(dll.ctrl, dll.prevValue);
      dll.wasChanged = false;
    }
  }

  #endif

  if (_cascaded_Changed
      || _menuIcons_Changed
      || _elimDup_Changed
      || _writeZone_Changed
      || _flags_Changed)
  {
    CContextMenuInfo ci;
    ci.Cascaded.Val = IsButtonCheckedBool(IDX_SYSTEM_CASCADED_MENU);
    ci.Cascaded.Def = _cascaded_Changed;

    ci.MenuIcons.Val = IsButtonCheckedBool(IDX_SYSTEM_ICON_IN_MENU);
    ci.MenuIcons.Def = _menuIcons_Changed;
    
    ci.ElimDup.Val = IsButtonCheckedBool(IDX_EXTRACT_ELIM_DUP);
    ci.ElimDup.Def = _elimDup_Changed;

    {
      int zoneIndex = (int)_zoneCombo.GetItemData_of_CurSel();
      if (zoneIndex <= 0)
        zoneIndex = -1;
      ci.WriteZone = (UInt32)(Int32)zoneIndex;
    }

    ci.Flags = 0;
    
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kMenuItems); i++)
      if (_listView.GetCheckState(i))
        ci.Flags |= kMenuItems[i].Flag;
    
    ci.Flags_Def = _flags_Changed;
    ci.Save();

    Clear_MenuChanged();
  }

  // UnChanged();

  return PSNRET_NOERROR;
}

void CMenuPage::OnNotifyHelp()
{
  ShowHelpWindow(kMenuTopic);
}

bool CMenuPage::OnButtonClicked(unsigned buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    #ifndef UNDER_CE
    case IDX_SYSTEM_INTEGRATE_TO_MENU:
    case IDX_SYSTEM_INTEGRATE_TO_MENU_2:
    {
      for (unsigned d = 0; d < 2; d++)
      {
        CShellDll &dll = _dlls[d];
        if (buttonID == dll.ctrl && !dll.Path.IsEmpty())
          dll.wasChanged = true;
      }
      break;
    }
    #endif

    case IDX_SYSTEM_CASCADED_MENU: _cascaded_Changed = true; break;
    case IDX_SYSTEM_ICON_IN_MENU: _menuIcons_Changed = true; break;
    case IDX_EXTRACT_ELIM_DUP: _elimDup_Changed = true; break;
    // case IDX_EXTRACT_WRITE_ZONE: _writeZone_Changed = true; break;

    case IDX_SYSTEM_MENU_CLASSIC:
    case IDX_SYSTEM_MENU_MODERN:
    case IDX_SYSTEM_MENU_BOTH:
    case IDX_SYSTEM_MENU_NONE:
      _menuMode_Changed = true;
      break;
      
    default:
      return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
  }
  
  Changed();
  return true;
}


bool CMenuPage::OnCommand(unsigned code, unsigned itemID, LPARAM param)
{
  if (code == CBN_SELCHANGE && itemID == IDC_SYSTEM_ZONE)
  {
    _writeZone_Changed = true;
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}


bool CMenuPage::OnNotify(UINT controlID, LPNMHDR lParam)
{
  if (lParam->hwndFrom == HWND(_listView))
  {
    switch (lParam->code)
    {
      case (LVN_ITEMCHANGED):
        return OnItemChanged((const NMLISTVIEW *)lParam);
    }
  }
  return CPropertyPage::OnNotify(controlID, lParam);
}


bool CMenuPage::OnItemChanged(const NMLISTVIEW *info)
{
  if (_initMode)
    return true;
  if ((info->uChanged & LVIF_STATE) != 0)
  {
    UINT oldState = info->uOldState & LVIS_STATEIMAGEMASK;
    UINT newState = info->uNewState & LVIS_STATEIMAGEMASK;
    if (oldState != newState)
    {
      _flags_Changed = true;
      Changed();
    }
  }
  return true;
}

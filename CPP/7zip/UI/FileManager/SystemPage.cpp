// SystemPage.cpp

#include "StdAfx.h"

#include "../../../Common/MyWindows.h"

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <shlobj.h>
#else
#include <ShlObj.h>
#endif

#include "../../../Common/Defs.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/Registry.h"

#include <shellapi.h>

#include "HelpUtils.h"
#include "IFolder.h"
#include "LangUtils.h"
#include "PropertyNameRes.h"
#include "ShellIntegrationModern.h"
#include "SystemPage.h"
#include "SystemPageRes.h"

using namespace NWindows;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

#ifdef Z7_LANG
static const UInt32 kLangIDs[] =
{
  IDT_SYSTEM_ASSOCIATE
};
#endif

#define kSystemTopic "FM/options.htm#system"

CSysString CModifiedExtInfo::GetString() const
{
  const char *s;
  if (State == kExtState_7Zip)
    s = "7-Zip ZS";
  else if (State == kExtState_Clear)
    s = "";
  else if (Other7Zip)
    s = "[7-Zip ZS]";
  else
    return ProgramKey;
  return CSysString (s);
}


int CSystemPage::AddIcon(const UString &iconPath, int iconIndex)
{
  if (iconPath.IsEmpty())
    return -1;
  if (iconIndex == -1)
    iconIndex = 0;
  
  HICON hicon;
  
  #ifdef UNDER_CE
  ExtractIconExW(iconPath, iconIndex, NULL, &hicon, 1);
  if (!hicon)
  #else
  // we expand path from REG_EXPAND_SZ registry item.
  UString path;
  const DWORD size = MAX_PATH + 10;
  const DWORD needLen = ::ExpandEnvironmentStringsW(iconPath, path.GetBuf(size + 2), size);
  path.ReleaseBuf_CalcLen(size);
  if (needLen == 0 || needLen >= size)
    path = iconPath;
  const UINT num = ExtractIconExW(path, iconIndex, NULL, &hicon, 1);
  if (num != 1 || !hicon)
  #endif
    return -1;
  
  _imageList.AddIcon(hicon);
  DestroyIcon(hicon);
  return (int)(_numIcons++);
}


void CSystemPage::RefreshListItem(unsigned group, unsigned listIndex)
{
  const CAssoc &assoc = _items[GetRealIndex(listIndex)];
  _listView.SetSubItem(listIndex, group + 1, assoc.Pair[group].GetString());
  LVITEMW newItem;
  memset(&newItem, 0, sizeof(newItem));
  newItem.iItem = (int)listIndex;
  newItem.mask = LVIF_IMAGE;
  newItem.iImage = assoc.GetIconIndex();
  _listView.SetItem(&newItem);
}


void CSystemPage::ChangeState(unsigned group, const CUIntVector &indices)
{
  if (indices.IsEmpty())
    return;

  bool thereAreClearItems = false;
  unsigned counters[3] = { 0, 0, 0 };
  
  unsigned i;
  for (i = 0; i < indices.Size(); i++)
  {
    const CModifiedExtInfo &mi = _items[GetRealIndex(indices[i])].Pair[group];
    int state = kExtState_7Zip;
    if (mi.State == kExtState_7Zip)
      state = kExtState_Clear;
    else if (mi.State == kExtState_Clear)
    {
      thereAreClearItems = true;
      if (mi.Other)
        state = kExtState_Other;
    }
    counters[state]++;
  }

  int state = kExtState_Clear;
  if (counters[kExtState_Other] != 0)
    state = kExtState_Other;
  else if (counters[kExtState_7Zip] != 0)
    state = kExtState_7Zip;
  
  for (i = 0; i < indices.Size(); i++)
  {
    unsigned listIndex = indices[i];
    CAssoc &assoc = _items[GetRealIndex(listIndex)];
    CModifiedExtInfo &mi = assoc.Pair[group];
    bool change = false;
    
    switch (state)
    {
      case kExtState_Clear: change = true; break;
      case kExtState_Other: change = mi.Other; break;
      default: change = !(mi.Other && thereAreClearItems); break;
    }
    
    if (change)
    {
      mi.State = state;
      RefreshListItem(group, listIndex);
    }
  }
  
  _needSave = true;
  Changed();
}


bool CSystemPage::OnInit()
{
  _needSave = false;

#ifdef Z7_LANG
  LangSetDlgItems(*this, kLangIDs, Z7_ARRAY_SIZE(kLangIDs));
#endif

  _listView.Attach(GetItem(IDL_SYSTEM_ASSOCIATE));
  _listView.SetUnicodeFormat();
  DWORD newFlags = LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  _numIcons = 0;
  _imageList.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);

  _listView.SetImageList(_imageList, LVSIL_SMALL);

  _listView.InsertColumn(0, LangString(IDS_PROP_FILE_TYPE), 80);

  UString s;

  #if NUM_EXT_GROUPS == 1
    s = "Program";
  #else
    #ifndef UNDER_CE
      const unsigned kSize = 256;
      BOOL res;

      DWORD size = kSize;

      #ifndef _UNICODE
      if (!g_IsNT)
      {
        AString s2;
        res = GetUserNameA(s2.GetBuf(size), &size);
        s2.ReleaseBuf_CalcLen(MyMin((unsigned)size, kSize));
        s = GetUnicodeString(s2);
      }
      else
      #endif
      {
        res = GetUserNameW(s.GetBuf(size), &size);
        s.ReleaseBuf_CalcLen(MyMin((unsigned)size, kSize));
      }
    
      if (!res)
    #endif
        s = "Current User";
  #endif

  LV_COLUMNW ci;
  ci.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
  ci.cx = 152;
  ci.fmt = LVCFMT_CENTER;
  ci.pszText = s.Ptr_non_const();
  ci.iSubItem = 1;
  _listView.InsertColumn(1, &ci);

  #if NUM_EXT_GROUPS > 1
  {
    LangString(IDS_SYSTEM_ALL_USERS, s);
    ci.pszText = s.Ptr_non_const();
    ci.iSubItem = 2;
    _listView.InsertColumn(2, &ci);
  }

  {
    /* 7-Zip ZS: the effective default app of Windows 10+ (UserChoice) is shown
       in an extra column, because the columns above only hold the classic ProgID
       which Windows ignores as soon as a UserChoice exists. */
    UString t (L"\u7CFB\u7EDF\u9ED8\u8BA4\u7A0B\u5E8F");   // "system default app"
    ci.pszText = t.Ptr_non_const();
    ci.iSubItem = 3;
    ci.cx = 140;
    _listView.InsertColumn(3, &ci);
  }
  #endif

  _extDB.Read();
  _items.Clear();

  FOR_VECTOR (i, _extDB.Exts)
  {
    const CExtPlugins &extInfo = _extDB.Exts[i];

    LVITEMW item;
    item.iItem = (int)i;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    item.lParam = (LPARAM)i;
    item.iSubItem = 0;
    // ListView always uses internal iImage that is 0 by default?
    // so we always use LVIF_IMAGE.
    item.iImage = -1;
    item.pszText = extInfo.Ext.Ptr_non_const();

    CAssoc assoc;
    const CPluginToIcon &plug = extInfo.Plugins[0];
    assoc.SevenZipImageIndex = AddIcon(plug.IconPath, plug.IconIndex);

    CSysString texts[NUM_EXT_GROUPS];
    unsigned g;
    for (g = 0; g < NUM_EXT_GROUPS; g++)
    {
      CModifiedExtInfo &mi = assoc.Pair[g];
      mi.ReadFromRegistry(GetHKey(g), GetSystemString(extInfo.Ext));
      mi.SetState(plug.IconPath);
      mi.ImageIndex = AddIcon(mi.IconPath, mi.IconIndex);
      texts[g] = mi.GetString();
    }
    item.iImage = assoc.GetIconIndex();
    const int itemIndex = _listView.InsertItem(&item);
    for (g = 0; g < NUM_EXT_GROUPS; g++)
      _listView.SetSubItem((unsigned)itemIndex, 1 + g, texts[g]);
    _items.Add(assoc);
  }
  
  if (_listView.GetItemCount() > 0)
    _listView.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);

  UpdateSystemDefaults();

  /* 7-Zip ZS: the dialog grows with the DPI scaling while the list columns keep
     their pixel widths, so the buttons are placed above the column they act on
     (in the template they have fixed coordinates). */
  {
    RECT rList;
    ::GetWindowRect(HWND(_listView), &rList);
    POINT pList = { rList.left, rList.top };
    ::ScreenToClient(*this, &pList);

    int x = pList.x;
    for (unsigned g = 0; g < NUM_EXT_GROUPS; g++)
    {
      const HWND h = GetItem(g == 0 ? IDB_SYSTEM_CURRENT : IDB_SYSTEM_ALL);
      RECT rb;
      ::GetWindowRect(h, &rb);
      const int w = (int)::SendMessage(HWND(_listView), LVM_GETCOLUMNWIDTH, (WPARAM)(g + 1), 0);
      ::SetWindowPos(h, NULL, x, pList.y - (rb.bottom - rb.top) - 2, w, rb.bottom - rb.top,
          SWP_NOZORDER | SWP_NOACTIVATE);
      x += w + 4;
    }
  }

  return CPropertyPage::OnInit();
}


/* Opens the Windows settings for the default apps.

   Windows 11 does not let an application change the default app of an extension:
   SHOpenWithDialog only answers "go to Settings > Apps > Default apps" (measured
   on build 26200 with a bare extension and with a real file of that type), the
   UserChoice value itself is protected by a hash, and this fork has no entry in
   HKLM\SOFTWARE\RegisteredApplications that IApplicationAssociationRegistrationUI
   needs. So the settings page is opened - the workflow that Windows asks for. */
void CSystemPage::OpenDefaultAppDialog(unsigned listIndex)
{
  const unsigned realIndex = GetRealIndex(listIndex);
  if (realIndex >= _extDB.Exts.Size())
    return;

  ::ShellExecuteW(*this, L"open", L"ms-settings:defaultapps", NULL, NULL, SW_SHOWNORMAL);
}


/* Reads the effective default app (Windows 10+ "UserChoice") of every extension
   and shows it in the last column. An application cannot write that value - it
   is protected by a hash, only the user can change it (Windows settings, "open
   with") - so this column exists to show what really happens on a double click
   instead of showing the classic ProgID only. */
void CSystemPage::UpdateSystemDefaults()
{
  const unsigned kColumn = 1 + NUM_EXT_GROUPS;

  FOR_VECTOR (i, _items)
  {
    CAssoc &assoc = _items[i];
    assoc.SystemDefault.Empty();

    UString keyName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.";
    keyName += _extDB.Exts[i].Ext;
    keyName += L"\\UserChoice";

    NRegistry::CKey key;
    if (key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) == ERROR_SUCCESS)
    {
      UString progId;
      if (key.QueryValue(L"ProgId", progId) == ERROR_SUCCESS && !progId.IsEmpty())
      {
        UString command;
        {
          NRegistry::CKey keyCmd;
          if (keyCmd.Open(HKEY_CLASSES_ROOT, progId + UString(L"\\shell\\open\\command"), KEY_READ) == ERROR_SUCCESS)
            keyCmd.QueryValue(NULL, command);
        }

        if (command.Find(L"7zFM.exe") >= 0)
          assoc.SystemDefault = L"7-Zip ZS";
        else
        {
          UString title;
          {
            NRegistry::CKey keyTitle;
            if (keyTitle.Open(HKEY_CLASSES_ROOT, progId, KEY_READ) == ERROR_SUCCESS)
              keyTitle.QueryValue(NULL, title);
          }
          assoc.SystemDefault = title.IsEmpty() ? progId : title;
        }
      }
    }

    _listView.SetSubItem(i, kColumn, assoc.SystemDefault);
  }
}


LONG CSystemPage::OnSetActive()
{
  /* the default app can be changed outside of 7-Zip (Windows settings or another
     archiver), so the column is refreshed whenever this page is shown */
  UpdateSystemDefaults();
  return false;   // false = OK
}


static UString GetProgramCommand()
{
  UString s ('\"');
  s += fs2us(NDLL::GetModuleDirPrefix());
  s += "7zFM.exe\" \"%1\"";
  return s;
}


/* Applies one entry of "-AssocAll=..." to HKEY_LOCAL_MACHINE (the "all users"
   group of the system page). */
static void Apply_AssocOne(bool add, const UString &ext, const CExtDatabase &extDB, const UString &command, LONG &res)
{
  if (add)
  {
    const CExtPlugins *found = NULL;
    FOR_VECTOR (i, extDB.Exts)
    {
      if (extDB.Exts[i].Ext.IsEqualTo_NoCase(ext))
      {
        found = &extDB.Exts[i];
        break;
      }
    }
    if (!found || found->Plugins.IsEmpty())
      return;

    UString title = found->Ext;
    title += " Archive";
    const CPluginToIcon &plug = found->Plugins[0];
    const LONG r = NRegistryAssoc::AddShellExtensionInfo(HKEY_LOCAL_MACHINE, ext,
        title, command, plug.IconPath, plug.IconIndex);
    if (r != ERROR_SUCCESS)
      res = r;
  }
  else
  {
    const LONG r = NRegistryAssoc::DeleteShellExtensionInfo(HKEY_LOCAL_MACHINE, ext);
    if (r != ERROR_SUCCESS)
      res = r;
  }
}


int ApplyAssocAll_FromCommandLine(const wchar_t *spec)
{
  if (!spec)
    return 1;
  if (wcsncmp(spec, L"-AssocAll=", 10) == 0)
    spec += 10;

  CExtDatabase extDB;
  extDB.Read();
  const UString command = GetProgramCommand();

  LONG res = ERROR_SUCCESS;
  bool add = false;
  UString cur;

  for (;;)
  {
    const wchar_t c = *spec;
    if (c == 0 || c == L'+' || c == L'-' || c == L',')
    {
      if (!cur.IsEmpty())
      {
        Apply_AssocOne(add, cur, extDB, command, res);
        cur.Empty();
      }
      if (c == 0)
        break;
      if (c != L',')
        add = (c == L'+');
    }
    else
      cur += c;
    spec++;
  }

  return res == ERROR_SUCCESS ? 0 : 1;
}


LONG CSystemPage::OnApply()
{
  if (!_needSave)
    return PSNRET_NOERROR;

  const UString command = GetProgramCommand();

  /* The "all users" group (group 1) writes to HKEY_LOCAL_MACHINE and therefore
     needs administrator rights. Those changes are collected and given to an
     elevated copy of this program: one UAC prompt, the program itself does not
     have to be restarted as administrator. */
  UString assocAdd, assocDel;
  bool allUsersDone = false;
  if (!NShellIntegrationModern::Is_Process_Elevated())
  {
    FOR_VECTOR (listIndex, _extDB.Exts)
    {
      const unsigned realIndex = GetRealIndex(listIndex);
      const CModifiedExtInfo &mi = _items[realIndex].Pair[1];
      if (mi.OldState == mi.State)
        continue;
      const UString &ext = _extDB.Exts[realIndex].Ext;
      if (mi.State == kExtState_7Zip)
      {
        if (!assocAdd.IsEmpty())
          assocAdd.Add_Char(',');
        assocAdd += ext;
      }
      else if (mi.State == kExtState_Clear)
      {
        if (!assocDel.IsEmpty())
          assocDel.Add_Char(',');
        assocDel += ext;
      }
    }

    if (!assocAdd.IsEmpty() || !assocDel.IsEmpty())
    {
      UString spec (L"-AssocAll=");
      if (!assocAdd.IsEmpty())
      {
        spec.Add_Char('+');
        spec += assocAdd;
      }
      if (!assocDel.IsEmpty())
      {
        spec.Add_Char('-');
        spec += assocDel;
      }

      UString error;
      if (NShellIntegrationModern::Run_Elevated_Self(spec, error) == S_OK)
        allUsersDone = true;
      else
      {
        if (error.IsEmpty())
          error = L"the elevated operation failed";
        MessageBoxW(*this, error.Ptr(), L"7-Zip ZS", MB_ICONERROR);
      }
    }
  }

  if (allUsersDone)
  {
    // the elevated copy has done the group-1 part already
    FOR_VECTOR (listIndex, _extDB.Exts)
    {
      CModifiedExtInfo &mi = _items[GetRealIndex(listIndex)].Pair[1];
      mi.OldState = mi.State;
    }
  }

  LONG res = 0;

  FOR_VECTOR (listIndex, _extDB.Exts)
  {
    unsigned realIndex = GetRealIndex(listIndex);
    const CExtPlugins &extInfo = _extDB.Exts[realIndex];
    CAssoc &assoc = _items[realIndex];

    for (unsigned g = 0; g < NUM_EXT_GROUPS; g++)
    {
      CModifiedExtInfo &mi = assoc.Pair[g];
      HKEY key = GetHKey(g);
      
      if (mi.OldState != mi.State)
      {
        LONG res2 = 0;
        
        if (mi.State == kExtState_7Zip)
        {
          UString title = extInfo.Ext;
          title += " Archive";
          const CPluginToIcon &plug = extInfo.Plugins[0];
          res2 = NRegistryAssoc::AddShellExtensionInfo(key, GetSystemString(extInfo.Ext),
              title, command, plug.IconPath, plug.IconIndex);
        }
        else if (mi.State == kExtState_Clear)
          res2 = NRegistryAssoc::DeleteShellExtensionInfo(key, GetSystemString(extInfo.Ext));
        
        if (res == 0)
          res = res2;
        if (res2 == 0)
          mi.OldState = mi.State;
        
        mi.State = mi.OldState;
        RefreshListItem(g, listIndex);
      }
    }
  }
  
  #ifndef UNDER_CE
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  #endif

  WasChanged = true;

  _needSave = false;
  
  if (res != 0)
    MessageBoxW(*this, NError::MyFormatMessage(res), L"7-Zip ZS", MB_ICONERROR);
  
  return PSNRET_NOERROR;
}


void CSystemPage::OnNotifyHelp()
{
  ShowHelpWindow(kSystemTopic);
}


bool CSystemPage::OnButtonClicked(unsigned buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    /*
    case IDC_SYSTEM_SELECT_ALL:
      _listView.SelectAll();
      return true;
    */
    case IDB_SYSTEM_CURRENT:
    case IDB_SYSTEM_ALL:
      ChangeState(buttonID == IDB_SYSTEM_CURRENT ? 0 : 1);
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}


bool CSystemPage::OnNotify(UINT controlID, LPNMHDR lParam)
{
  if (lParam->hwndFrom == HWND(_listView))
  {
    switch (lParam->code)
    {
      case NM_RETURN:
      {
        ChangeState(0);
        return true;
      }

      case NM_DBLCLK:
      {
        /* 7-Zip ZS: double click sets the effective default app of Windows 10+
           (the Windows "open with" dialog, which writes the protected
           UserChoice) */
        NMITEMACTIVATE *item = (NMITEMACTIVATE *)lParam;
        if (item->iItem >= 0)
          OpenDefaultAppDialog((unsigned)item->iItem);
        return true;
      }

      case NM_CLICK:
      {
        #ifdef UNDER_CE
        NMLISTVIEW *item = (NMLISTVIEW *)lParam;
        #else
        NMITEMACTIVATE *item = (NMITEMACTIVATE *)lParam;
        if (item->uKeyFlags == 0)
        #endif
        {
          if (item->iItem >= 0)
          {
            // unsigned realIndex = GetRealIndex(item->iItem);
            if (item->iSubItem >= 1 && item->iSubItem <= 2)
            {
              CUIntVector indices;
              indices.Add((unsigned)item->iItem);
              ChangeState(item->iSubItem < 2 ? 0 : 1, indices);
            }
          }
        }
        break;
      }
      
      case LVN_KEYDOWN:
      {
        if (OnListKeyDown(LPNMLVKEYDOWN(lParam)))
          return true;
        break;
      }
      
      /*
      case NM_RCLICK:
      case NM_DBLCLK:
      case LVN_BEGINRDRAG:
        // PostMessage(kRefreshpluginsListMessage, 0);
        PostMessage(kUpdateDatabase, 0);
        break;
      */
    }
  }
  return CPropertyPage::OnNotify(controlID, lParam);
}


void CSystemPage::ChangeState(unsigned group)
{
  CUIntVector indices;
  
  int itemIndex = -1;
  while ((itemIndex = _listView.GetNextSelectedItem(itemIndex)) != -1)
    indices.Add((unsigned)itemIndex);
  
  if (indices.IsEmpty())
    FOR_VECTOR (i, _items)
      indices.Add(i);
  
  ChangeState(group, indices);
}


bool CSystemPage::OnListKeyDown(LPNMLVKEYDOWN keyDownInfo)
{
  bool ctrl = IsKeyDown(VK_CONTROL);
  bool alt = IsKeyDown(VK_MENU);

  if (alt)
    return false;

  if ((ctrl && keyDownInfo->wVKey == 'A')
      || (!ctrl && keyDownInfo->wVKey == VK_MULTIPLY))
  {
    _listView.SelectAll();
    return true;
  }

  switch (keyDownInfo->wVKey)
  {
    case VK_SPACE:
    case VK_ADD:
    case VK_SUBTRACT:
    case VK_SEPARATOR:
    case VK_DIVIDE:

    #ifndef UNDER_CE
    case VK_OEM_PLUS:
    case VK_OEM_MINUS:
    #endif

      if (!ctrl)
      {
        ChangeState(keyDownInfo->wVKey == VK_SPACE ? 0 : 1);
        return true;
      }
      break;
  }

  return false;
}

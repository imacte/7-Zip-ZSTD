// ShellOperationWait.h
#ifndef ZIP7_INC_SHELL_OPERATION_WAIT_H
#define ZIP7_INC_SHELL_OPERATION_WAIT_H

#include <windows.h>
#include <commctrl.h>

// Prevent Apply/Cancel/Close reentry while pumping messages during a registry
// transaction. The owner stays alive until the helper and any rollback finish.
class CShellOperationGuard
{
  HWND _window;
  bool _enabled;
  static LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
      UINT_PTR, DWORD_PTR)
  {
    if (msg == WM_CLOSE || msg == WM_COMMAND || msg == WM_NOTIFY
        || (msg == WM_SYSCOMMAND && (wp & 0xfff0) == SC_CLOSE)) return 0;
    return DefSubclassProc(hwnd, msg, wp, lp);
  }
public:
  explicit CShellOperationGuard(HWND page): _window(GetAncestor(page, GA_ROOT)),
      _enabled(_window && IsWindowEnabled(_window))
  {
    if (_enabled)
    {
      SetWindowSubclass(_window, Proc, (UINT_PTR)this, 0);
      EnableWindow(_window, FALSE);
    }
  }
  ~CShellOperationGuard()
  {
    if (_enabled)
    {
      RemoveWindowSubclass(_window, Proc, (UINT_PTR)this);
      EnableWindow(_window, TRUE);
    }
  }
private:
  CShellOperationGuard(const CShellOperationGuard &);
  CShellOperationGuard &operator=(const CShellOperationGuard &);
};

// Do not time out and roll back while the helper is still writing. Pump the
// caller's messages instead, so painting and unrelated windows remain live.
inline DWORD WaitForShellOperation(HANDLE handle)
{
  bool quit = false;
  int quitCode = 0;
  DWORD result;
  for (;;)
  {
    result = MsgWaitForMultipleObjectsEx(1, &handle, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    if (result != WAIT_OBJECT_0 + 1) break;
    MSG msg;
    // Bound each batch so a busy queue cannot starve the completion handle.
    for (unsigned i = 0; i < 64 && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE); i++)
    {
      if (msg.message == WM_QUIT) { quit = true; quitCode = (int)msg.wParam; }
      else { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
  }
  if (quit) PostQuitMessage(quitCode);
  return result;
}
#endif

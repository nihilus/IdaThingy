#ifndef __FIND_TAPPLICATION__05102007__
#define __FIND_TAPPLICATION__05102007__

#include <windows.h>
#include <tchar.h>

/*

FindTApplication utility class allows you to find the handle of the TApplication of a given VCL program.

This code is copyright lallous <lallousx86@yahoo.com>
You may use the code freely, however modification or distribution of the code needed prior permission from the author.

*/

class FindTApplication
{
private:
  HWND  _hwndResult;
  DWORD _childPID;

  static BOOL CALLBACK EnumProc(HWND hwnd,LPARAM lParam)
  {
    DWORD pid;
    FindTApplication *_this = (FindTApplication *)lParam;

    // Different process?
    ::GetWindowThreadProcessId(hwnd, &pid);
    
    if (pid != _this->_childPID)
      return TRUE;

    TCHAR className[MAX_PATH];

    if (GetClassName(hwnd, className, MAX_PATH) != 0)
    {
      if (_tcscmp(className, _T("TApplication")) == 0)
      {
        _this->_hwndResult = hwnd;
        return FALSE;
      }
    }
    return TRUE;
  }

public:
  FindTApplication()
  {
    _hwndResult = 0;
  }
  static HWND Exec(HWND childWin)
  {
    // Not a window?
    if (!IsWindow(childWin))
      return 0;

    FindTApplication *_this = new FindTApplication();
    
    GetWindowThreadProcessId(childWin, &_this->_childPID);

    ::EnumWindows(EnumProc, (LPARAM)_this);

    HWND result = _this->_hwndResult;

    delete _this;
    return result;
  }
};


#endif

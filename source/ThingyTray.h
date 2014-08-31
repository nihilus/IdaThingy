#ifndef __THINGY_TRAY__
#define __THINGY_TRAY__

#include <windows.h>
#include "lib\TrayIconHandler.h"
#include "lib\FindTApplication.hpp"

class CThingyTray: public CTrayIconHandler
{
private:
  int m_groupAutoAn, m_idxBusy, m_idxIdle;
  bool m_bMinimized;
  DWORD m_nLastProcessPriority;
  int m_nInitCount;
public:
  enum
  {
    idxIdle = 0,
    idxBusy = 1
  };

  CThingyTray();
  int     Init();
  int     Stop();
  void    ghAutoAn(int grId, UINT msg, UINT frame);
  LRESULT OnWMCommand(WPARAM wParam, LPARAM lParam);
  LRESULT OnWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
  void    DoIdle(bool bBusy = false);
  void    DoBusy();
  void    DoMinimizeToTray();
  void    DoMaximizeFromTray();
  bool    IsInTray() { return m_bMinimized; }
};

#endif
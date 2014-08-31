#include <ida.hpp>
#include <loader.hpp>
#include <auto.hpp>

#include "ThingyTray.h"
#include "util.h"
#include "resource.h"

extern HINSTANCE g_hInstance;

CThingyTray::CThingyTray()
{
  m_nInitCount = 0;
}

LRESULT CThingyTray::OnWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return CTrayIconHandler::OnWindowMessage(hWnd, Msg, wParam , lParam);
}

int CThingyTray::Init()
{
  int ret  = CTrayIconHandler::errOk;

  if (m_nInitCount++ > 0)
    return ret;

  m_bMinimized = false;

  if (get_ida_hwnd() == NULL)
    return CTrayIconHandler::errNoId;

  ret = CTrayIconHandler::Init();

  SetHInstance(g_hInstance);

  groupCreate(m_groupAutoAn, GroupHandler(&CThingyTray::ghAutoAn));

  groupAddIcon(m_groupAutoAn, IDI_IDA_IDLE, "IDA is Idle");
  groupAddIcon(m_groupAutoAn, IDI_IDA_BUSY, "IDA is Busy");

  return ret;
}

void CThingyTray::ghAutoAn(int grId, UINT msg, UINT frame)
{
  if (msg == WM_LBUTTONUP)
  {
    DoMaximizeFromTray();
  }
}

LRESULT CThingyTray::OnWMCommand(WPARAM wParam, LPARAM lParam)
{
  /*
  int act = gsPause;

  if (wParam == IDM_PAUSE1)
  {
  if (groupGetAnimationState(m_gr1) == gsPaused)
  act = gsPlay;
  groupAnimate(m_gr1, act, 0);
  }
  else if (wParam == IDM_PAUSE2)
  {
  if (groupGetAnimationState(m_gr2) == gsPaused)
  act = gsPlay;
  groupAnimate(m_gr2, act, 0);
  }
  else if (wParam == IDM_SHOWBALLON)
  {
  showBalloonTip(groupGetIconId(m_gr1), "Hey", "what", 1);
  }
  */
  return 0;
}

void CThingyTray::DoIdle(bool bBusy)
{
  groupShow(m_groupAutoAn, bBusy ? idxBusy : idxIdle);

  char msg[QMAXPATH];
  int id = groupGetIconId(m_groupAutoAn);
  qsnprintf(msg, QMAXPATH, "'%s' is now %s!", database_idb, bBusy ? "busy" : "idle");
  updateIcon(id, cteKeepIcon, msg);

  showBalloonTip(id, msg, "Status", 1);
}

void CThingyTray::DoMinimizeToTray()
{
  HWND h = get_ida_hwnd();
  HWND h2 = FindTApplication::Exec(h);

  if (!::IsWindow(h) || !::IsWindow(h2))
    return;

  m_nLastProcessPriority = ::GetPriorityClass(::GetCurrentProcess());

  groupShow(m_groupAutoAn,  autoIsOk() ? idxIdle : idxBusy);

  char msg[QMAXPATH];
  int id = groupGetIconId(m_groupAutoAn);
  qsnprintf(msg, QMAXPATH, "'%s' is now %s!", database_idb, !autoIsOk() ? "busy" : "idle");
  updateIcon(id, cteKeepIcon, msg);

  ::ShowWindow(h, SW_HIDE);
  ::ShowWindow(h2, SW_HIDE);

  m_bMinimized = true;
}

int CThingyTray::Stop()
{
  if (--m_nInitCount > 0)
    return CTrayIconHandler::errOk;
  return CTrayIconHandler::Stop();
}

void CThingyTray::DoMaximizeFromTray()
{
  HWND h = get_ida_hwnd();
  HWND h2 = FindTApplication::Exec(h);

  if (!::IsWindow(h) || !::IsWindow(h2))
    return;

  ::ShowWindow(h, SW_SHOW);
  ::ShowWindow(h2, SW_SHOW);

  groupHide(m_groupAutoAn);

  ::SetPriorityClass(::GetCurrentProcess(), m_nLastProcessPriority);

  ::SetForegroundWindow(h2);

  m_bMinimized = false;
}

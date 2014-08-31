/* ----------------------------------------------------------------------------- 
* Copyright (c) Elias "lallous" Bachaalany <lallousx86@yahoo.com>
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
* ----------------------------------------------------------------------------- 

History
---------
04/19/2004 - Initial version
05/05/2005 - checked all code, commented everything and added to libraries (for future reuse)
12/07/2006 - Added Wait() to wait till the trayicon is stopped (via Stop())

Todo
-----

What
-----

A group is a set of icons, where each icon is displayed in the place of another when flipped or animated.
Thus, all icons in the same group have (technically) the same icon-id but each icon replaces the other

*/
#include "TrayIconHandler.h"

#pragma warning (disable: 4390)

#ifndef CALL_MEMBER_FN
#define CALL_MEMBER_FN(object, ptrToMember)  ((object).*(ptrToMember))
#endif

#ifdef __DEBUGTRAYICONHANDLER
#define DEBUGP(x) printf x
#else
#define DEBUGP(x)
#endif

// This message received when the task bar is re-created
UINT CTrayIconHandler::m_WM_TASKBARCREATED = 0;

// This constant can be passed to tell the appropriate function that
// we don't need to update the icon
const HICON CTrayIconHandler::cteKeepIcon = (HICON) -1;

//-------------------------------------------------------------------------------------
// Inits a protected block
bool CTrayIconHandler::BeginRaceProtection()
{
  if (m_hRaceProtection)
  {
    WaitForSingleObject(m_hRaceProtection, INFINITE);
    return true;
  }

  // Create and acquire mutex
  m_hRaceProtection = ::CreateMutex(NULL, TRUE, NULL);
  if (!m_hRaceProtection)
    return false;
  return true;
}

//-------------------------------------------------------------------------------------
// Releases the protected block
void CTrayIconHandler::EndRaceProtection(bool bTerminate)
{
  ::ReleaseMutex(m_hRaceProtection);
  if (bTerminate)
  {
    ::CloseHandle(m_hRaceProtection);
    m_hRaceProtection = NULL;
  }
}

//-------------------------------------------------------------------------------------
// ctor
CTrayIconHandler::CTrayIconHandler(bool bDebug)
{
  m_hInstance = ::GetModuleHandle(NULL);

  m_hTrayWindow = NULL;

  // Automatic numbers assigned to icons and groups
  m_nTrayIconGroupSerial = m_nTrayIconSerial = 0;

  m_hTrayIconThread  = m_hRaceProtection = NULL;
  
  m_bDebug = bDebug;

  m_nError = errOk;

  // Create event for error indication / polling
  m_hPollError = ::CreateEvent(NULL, TRUE, FALSE, NULL);

  // Message sent by the system when the taskbar is [re]created
  m_WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));

  // Assign default window-name
  _tcscpy(m_szWindowName, _T("CTrayIconHandlerWindow"));
}

//-------------------------------------------------------------------------------------
// Creates the hidden window that will receive/handle tray icons
// When bUnmake, it will Unregister the window class / destroy the window
int CTrayIconHandler::MakeWindow(bool bUnmake)
{
  HWND hwnd;
  WNDCLASSEX wcl;

  // the class name
  LPCTSTR szClassName = _T("TIHW-CLS-F18A5ECF-A5CC-4f0f-8AD8-E7B37CABB361");

  // Should we destroy the window?
  if (bUnmake)
  {
    ::UnregisterClass(szClassName, m_hInstance);
    ::DestroyWindow(m_hTrayWindow);
    m_hTrayWindow = NULL;
    return errOk;
  }

  // Set the window class
  wcl.cbSize         = sizeof(WNDCLASSEX);
  wcl.cbClsExtra     = 0;
  wcl.cbWndExtra     = 0;
  wcl.hbrBackground  = NULL;
  wcl.lpszClassName  = szClassName;
  wcl.lpszMenuName   = NULL;
  wcl.hCursor        = NULL;
  wcl.hIcon          = NULL;
  wcl.hIconSm        = NULL;
  wcl.hInstance      = m_hInstance;
  wcl.lpfnWndProc    = WindowProc;
  wcl.style          = 0;

  // Failed to register class other than that the class already exists?
  if (!::RegisterClassEx(&wcl) && (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
    return errClassRegistration;

  // Create the window
  hwnd = ::CreateWindowEx(
    0,
    szClassName, 
    m_szWindowName, 
    WS_OVERLAPPEDWINDOW,
    0,
    0,
    100,
    100,
    HWND_DESKTOP,
    NULL,
    m_hInstance,
    (LPVOID)this);

  // Window creation failed ?
  if (!hwnd)
    return errWindowCreate;

  // Hide window or show it, depending on the debug state
  ::ShowWindow(hwnd, m_bDebug ? SW_SHOW : SW_HIDE);

  // Store 'this' into user data, so we can access it through the static WindowProc()
  ::SetWindowLong(hwnd, GWL_USERDATA, reinterpret_cast<LONG>(this));

  m_hTrayWindow = hwnd;

  return errOk;
}

//-------------------------------------------------------------------------------------
// Assign a new hInstance
void CTrayIconHandler::SetHInstance(HINSTANCE hInst)
{
  m_hInstance = hInst;
}

//-------------------------------------------------------------------------------------
// dtor()
CTrayIconHandler::~CTrayIconHandler()
{
  if (IsStarted())
    Stop();

  EndRaceProtection(true);
  ::CloseHandle(m_hPollError);
}

//-------------------------------------------------------------------------------------
// Stops the tray icon handler, deleting all the related icons
int CTrayIconHandler::Stop()
{
  if (!IsStarted())
    return errOk;

  // Hide/delete all icons
  hideAllIcons(true);

  // Hide/delete all groups
  groupHideAll(true);

  // Signal that we need to exit
  ::SendMessage(m_hTrayWindow, WM_DESTROY, 0, 0);

  // Wait for thread to terminate itself
  ::WaitForSingleObject(m_hTrayIconThread, 1000);

  // Close handle
  ::CloseHandle(m_hTrayIconThread);

  // Invalidate thread handle
  m_hTrayIconThread = NULL;

  // Destroy window
  MakeWindow(true);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Initializes thr tray icon handler
int CTrayIconHandler::Init()
{
  ::ResetEvent(m_hPollError);

  if (IsStarted())
    return errOk;

  // Create tray handler thread
  m_hTrayIconThread = 
    ::CreateThread(NULL, 
    0, 
    TrayIconThreadProc,
    reinterpret_cast<LPVOID>(this),
    NULL,
    NULL);

  // Thread creation failed?!
  if (!m_hTrayIconThread)
    return errThreadProc;

  // Wait for error code from inside the thread
  ::WaitForSingleObject(m_hPollError, INFINITE);

  // Unsuccessful?!
  if (m_nError != errOk)
  {
    // Wait for thread to terminate itself
    ::WaitForSingleObject(m_hTrayIconThread, INFINITE);

    // Close handle
    ::CloseHandle(m_hTrayIconThread);

    // Invalidate thread handle
    m_hTrayIconThread = NULL;
  }

  return errOk;
}

//-------------------------------------------------------------------------------------
// Given a tray-icon-id it returns the associated 'trayicondata_t' struct
bool CTrayIconHandler::findTrayIconDataById(int id, trayicondata_t **tid)
{
  trayicondata_map::iterator it = m_traymap.find(id);
  if (it == m_traymap.end())
    return false;

  *tid = &it->second;

  return true;
}

//-------------------------------------------------------------------------------------
// deletes the given trayicondata_t from the internal icon list
bool CTrayIconHandler::removeTrayIconDataById(int id)
{
  trayicondata_map::iterator pos = m_traymap.find(id);
  if (pos == m_traymap.end())
    return false;

  m_traymap.erase(pos);
  return true;
}

//-------------------------------------------------------------------------------------
// shows the given icon (by id)
int CTrayIconHandler::showIcon(const int id)
{
  trayicondata_t *tid;

  if (!findTrayIconDataById(id, &tid))
    return errNoId;

  tid->bVisible = true;

  ::Shell_NotifyIcon(NIM_ADD, &tid->nid);

  return errOk;
}

//-------------------------------------------------------------------------------------
// hides or deletes a given trayicon by its tid struct
int CTrayIconHandler::hideIcon(trayicondata_t &tid, bool bDelete)
{
  ::Shell_NotifyIcon(NIM_DELETE, &tid.nid);

  // reset its visible flag
  tid.bVisible = false;

  if (bDelete)
    return removeTrayIconDataById(tid.id);

  return errOk;
}

//-------------------------------------------------------------------------------------
// hides or delete a given icon by id
int CTrayIconHandler::hideIcon(const int id, bool bDelete)
{
  trayicondata_t *tid;

  if (!findTrayIconDataById(id, &tid))
    return errNoId;

  return hideIcon(*tid, bDelete);
}

//-------------------------------------------------------------------------------------
// some icons may have the 'visible' flag set yet they are not shown on the tray-bar
// This call will re-show all icons that have 'visible' flag set.
void CTrayIconHandler::showLostIcons()
{
  trayicondata_map::iterator it;

  // Walk in the whole single icons list
  for (it=m_traymap.begin();it != m_traymap.end();++it)
  {
    trayicondata_t tid = it->second;

    // is this icon visible?
    if (tid.bVisible)
    {
      showIcon(tid.id);
      if (m_bDebug)
        DEBUGP(("re-constructed icon:%d\n", tid.id));
    }
  }
}

//-------------------------------------------------------------------------------------
// This thread is responsible for dealing with the message pumping / window message loop
DWORD WINAPI CTrayIconHandler::TrayIconThreadProc(LPVOID Param)
{
  MSG msg;
  BOOL bRet;

  // Extract 'this'
  CTrayIconHandler *_this = reinterpret_cast<CTrayIconHandler *>(Param);

  // Failed to create window?
  if ((_this->m_nError = _this->MakeWindow()) != errOk)
  {
    ::SetEvent(_this->m_hPollError);
    return ERROR_SUCCESS;
  }

  ::SetEvent(_this->m_hPollError);

  // Window message loop
  while (bRet = ::GetMessage(&msg, _this->m_hTrayWindow, 0, 0))
  {
    if (bRet == -1)
      break;

    if (_this->m_bDebug)
      DEBUGP((". wm=%08X wParam=%04x lParam=%08X\n", msg.message, msg.wParam, msg.lParam));

    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  _this->m_nError = errOk;
  return ERROR_SUCCESS;
}

//-------------------------------------------------------------------------------------
// Hides all the icons
void CTrayIconHandler::hideAllIcons(bool bDelete)
{
  trayicondata_map::iterator it;

  while ( (it = m_traymap.begin()) != m_traymap.end())
  {
    trayicondata_t tid = it->second;
    hideIcon(tid, bDelete);
  }
}

//-------------------------------------------------------------------------------------
// Hides all the groups
void CTrayIconHandler::groupHideAll(bool bDelete)
{
  trayicongroup_map::iterator it;

  while ( (it = m_traygroup.begin()) != m_traygroup.end())
  {
    groupHide(it->first, bDelete);
  }
}

//-------------------------------------------------------------------------------------
// Returns an HICON from a given on-disk icon file
HICON CTrayIconHandler::hIconFromFile(LPCTSTR szIcon)
{
  HICON hIcon;
  hIcon = (HICON) ::LoadImage(NULL, szIcon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
  return hIcon;
}

//---------------------------------------------------------------------------------
// Adds an icon given its file name
int CTrayIconHandler::addIcon(int &id, LPCTSTR szIcon, bool bShow, LPTSTR szTip, IconHandler handler)
{
  HICON hIcon;

  // Get icon file
  hIcon = hIconFromFile(szIcon);

  if (hIcon == NULL)
    return errNoIcon;

  return addIcon(id, hIcon, bShow, szTip, handler);
}

//---------------------------------------------------------------------------------
// Adds an icon given its resource identifier
int CTrayIconHandler::addIcon(int &id, UINT IconId, bool bShow, LPTSTR szTip, IconHandler handler)
{
  HICON hIcon = ::LoadIcon(m_hInstance, MAKEINTRESOURCE(IconId));
  if (hIcon == NULL)
    return errNoIcon;

  return addIcon(id, hIcon, bShow, szTip, handler);
}

//---------------------------------------------------------------------------------
// adds a tray icon given the hIcon, szTip and handler
// it will return in 'id' the value you can use to further manage the added tray icon
int CTrayIconHandler::addIcon(int &id, HICON hIcon, bool bShow, LPTSTR szTip, IconHandler handler)
{
  trayicondata_t tid = {0};

  // assign a serial icon-id
  id = m_nTrayIconSerial++;

  // Doesn't belong to any group (pun intended! ;))
  tid.groupid = InvalidGroupId;
  tid.id = id;
  tid.bVisible = bShow;

  tid.nid.cbSize = sizeof(NOTIFYICONDATA);
  tid.nid.hWnd   = m_hTrayWindow;
  tid.nid.uFlags = NIF_ICON | NIF_MESSAGE | (szTip ? NIF_TIP : 0);
  tid.nid.hIcon  = hIcon;
  tid.nid.uID = id;
  tid.nid.uCallbackMessage = CALLBACKMESSAGE;

  if (szTip)
  {
    size_t n = (sizeof(tid.nid.szTip) / sizeof(TCHAR))-1;
    _tcsncpy(tid.nid.szTip, szTip, n);
    tid.nid.szTip[n] = TCHAR('\0');
  }

  if (handler)
    tid.handler = handler;

  // Add the entry
  m_traymap[id] = tid;

  if (bShow)
    return showIcon(id);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Updates a given icon by id
// changes its ICON and associated szTip
int CTrayIconHandler::updateIcon(const int id, HICON hIcon, LPTSTR szTip, IconHandler handler)
{
  trayicondata_t *tid;

  if (!findTrayIconDataById(id, &tid))
    return errNoId;

  tid->nid.uFlags = NIF_ICON | NIF_MESSAGE | (szTip ? NIF_TIP : 0);  

  if (hIcon != cteKeepIcon)
    tid->nid.hIcon = hIcon;

  if (szTip)
  {
    size_t n = (sizeof(tid->nid.szTip) / sizeof(TCHAR))-1;
    _tcsncpy(tid->nid.szTip, szTip, n);
    tid->nid.szTip[n] = TCHAR('\0');
  }

  ::Shell_NotifyIcon(NIM_MODIFY, &tid->nid);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Given an icon-id it returns the associated 'tid' struct
CTrayIconHandler::trayicondata_t *CTrayIconHandler::findTrayIconDataPtrById(int id)
{
  trayicondata_map::iterator it = m_traymap.find(id);
  if (it == m_traymap.end())
    return 0;

  return &it->second;
}

bool CTrayIconHandler::showPopupMenu(HINSTANCE hInst, HWND hWnd, UINT menuId, int menuPos)
{
  HMENU menu = ::LoadMenu(hInst, MAKEINTRESOURCE(menuId));

  if (menu == NULL)
    return false;

  HMENU popup = ::GetSubMenu(menu, menuPos);
  if (menu == NULL)
    return false;

  POINT pt = {0};
  RECT r = {0};

  ::GetCursorPos(&pt);
  return ::TrackPopupMenu(popup, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, &r) == TRUE ? true : false;
}

//-------------------------------------------------------------------------------------
// Its shows the given menu-item-id (from the resource)
// Using X and Y coords of the mouse at that given time
// The hWnd can be any handle belonging to the calling application
bool CTrayIconHandler::showPopupMenu(UINT menuId, int menuPos)
{
  return showPopupMenu(m_hInstance, m_hTrayWindow, menuId, menuPos);
}

//-------------------------------------------------------------------------------------
// Creates an icon group and returns the group id
int CTrayIconHandler::groupCreate(int &grId, GroupHandler handler)
{
  trayicongroup_t tig = {0};

  grId = m_nTrayIconGroupSerial++;

  tig.id       = grId;
  tig.state    = gsStopped;
  tig.handler  = handler;

  m_traygroup[grId] = tig;

  return errOk;
}

//-------------------------------------------------------------------------------------
// It flips through the icons in a given group-id
int CTrayIconHandler::groupFlipIcon(int grId)
{
  int err = errOk;

  trayicongroup_t *tig;

  // get the group information
  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  // Advance to next icon in the group
  tig->frame++;

  // Surpassed the frames count? then go to beginning
  if (tig->frame >= tig->frames.size())
    tig->frame = 0;

  // Show the group's icon by its frame
  groupShow(grId, tig->frame);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Given a group-data and a frame number, we retrieve the frame-data
int CTrayIconHandler::trayIconFrameDataFromFrameNumber(trayicongroup_t &tig, 
                                                       UINT frame, 
                                                       trayiconframe_t **tif)
{
  trayiconframe_list::iterator it = tig.frames.begin();

  std::advance(it, frame);

  if (it == tig.frames.end())
    return errNoFrame;

  *tif = &*it;

  return errOk;
}

//-------------------------------------------------------------------------------------
// Given a group id it returns the icongroupdata structure
bool CTrayIconHandler::findTrayIconGroupById(int grId, trayicongroup_t **tig)
{
  trayicongroup_map::iterator it = m_traygroup.find(grId);
  if (it == m_traygroup.end())
    return false;

  *tig = &it->second;

  return true;
}

//-------------------------------------------------------------------------------------
// This is a callback function that routes appropriate tray-icon related messages
// to appropriate registered handlers.
void CTrayIconHandler::routeMessageToHandler(int id, UINT msg)
{
  trayicondata_t *tid;

  // Do we have this icon-id ?
  if (!findTrayIconDataById(id, &tid))
    return;

  // This icon belongs to a group?
  if (tid->groupid != InvalidGroupId)
  {
    if (m_bDebug)
      DEBUGP(("handler() group(%d)\n", tid->groupid));

    // Do we have a handler associated with this group?
    trayicongroup_t *tig;
    if (findTrayIconGroupById(tid->groupid, &tig) && tig->handler)
    {
      // Call group handler
      CALL_MEMBER_FN(*this, tig->handler)(tig->id, msg, tig->frame);
    }
  }
  // This icon belongs to a single icon
  else if (tid->handler)
  {
    // call single icon handler
    CALL_MEMBER_FN(*this, tid->handler)(tid->id, msg);
  }
}

//-------------------------------------------------------------------------------------
// Returns the icon ID used by the given group
// Usually the group is just a set of icons showing at the same icon-id
int CTrayIconHandler::groupGetIconId(int grId)
{
  trayicongroup_t *tig;

  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  return tig->iconid;
}

//-------------------------------------------------------------------------------------
// Returns the group's animation state
// Which can be any of the gsXXXX members -> paused playing stopped
int CTrayIconHandler::groupGetAnimationState(int grId)
{
  trayicongroup_t *tig;

  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  return tig->state;
}

//-------------------------------------------------------------------------------------
// Adds an icon to the group given the icon-file-path and the szTip associated with the icon
int CTrayIconHandler::groupAddIcon(int grId, LPCTSTR szIcon, LPTSTR szTip)
{
  HICON hIcon = hIconFromFile(szIcon);
  if (hIcon == NULL)
    return errNoIcon;
  return groupAddIcon(grId, hIcon, szTip);
}

//-------------------------------------------------------------------------------------
// Adds an icon to the group given the icon-resource-id
int CTrayIconHandler::groupAddIcon(int grId, UINT IconId, LPTSTR szTip)
{
  HICON hIcon = ::LoadIcon(m_hInstance, MAKEINTRESOURCE(IconId));
  if (hIcon == NULL)
    return errNoIcon;
  return groupAddIcon(grId, hIcon, szTip);
}

//-------------------------------------------------------------------------------------
// Adds an icon to a given group
int CTrayIconHandler::groupAddIcon(int grId, HICON hIcon, LPTSTR szTip)
{
  trayicongroup_t *tig;

  // Group not there?
  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  trayiconframe_t tif;

  tif.hIcon = hIcon;

  if (szTip)
  {
    size_t n = (sizeof(tif.szTip) / sizeof(TCHAR)) - 1;
    _tcsncpy(tif.szTip, szTip, n);
    tif.szTip[n] = TCHAR('\0');
  }
  else
    tif.szTip[0] = TCHAR('\0');

  // First time addition?
  // If so, we need to create a normal ICON first
  if (tig->frames.empty())
  {
    int iconId, err;
    if ((err = addIcon(iconId, hIcon, false, szTip)) != errOk)
      return err;
    
    // Get the icon-data of the newly added icon
    trayicondata_t *tid;
    findTrayIconDataById(iconId, &tid);

    // Set the group id
    tid->groupid = grId;

    // initialize the members of the group
    tig->frame = 0;
    tig->iconid = iconId;
  }

  // Add this frame to the group's frame list
  tig->frames.push_back(tif);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Animates a give group using the specified 'delay'
// The 'action' can be either pause, play or stop animation
int CTrayIconHandler::groupAnimate(int grId, int action, int delay)
{
  int err = errOk;

  trayicongroup_t *tig;
  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  switch (action)
  {
  // pause animation?
  case gsPause:
    if (tig->state != gsStopped)
      tig->state = gsPaused;
    break;

  // play?
  case gsPlay:
    if (tig->state == gsPlaying)
      return errAlreadyPlaying;
    else if (tig->state == gsPaused)
      tig->state = gsPlaying;
    else if (tig->state == gsStopped)
    {
      // create a timer with an ID same as the group ID
      tig->timerid = ::SetTimer(GetHwnd(), tig->id, delay, NULL);
      tig->state = gsPlaying;
    }
    break;

  // stop animation?
  case gsStop:
    if ((tig->state == gsStopped) || (tig->timerid == 0))
      break;
    ::KillTimer(GetHwnd(), tig->timerid);
    tig->timerid = 0; // clear timer-id
    break;

  // restart animation?
  case gsRestart:
    if ((err = groupAnimate(grId, gsStop, 0)) != errOk)
      break;
    err = groupAnimate(grId, gsPlay, delay);
    break;
  }

  return err;
}

//-------------------------------------------------------------------------------------
// Window Procedure
LRESULT CALLBACK CTrayIconHandler::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  // Try to parse out the _this from the window-long information
  CTrayIconHandler *_this = reinterpret_cast<CTrayIconHandler *>(::GetWindowLong(hWnd, GWL_USERDATA));

  // WM_CREATE? then we can parse-out the _this pointer
  if (Msg == WM_CREATE)
  {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)(lParam);
    _this = reinterpret_cast<CTrayIconHandler *>(cs->lpCreateParams);
  }

  switch (Msg)
  {
  case WM_DESTROY:
    ::PostQuitMessage(0);
    break;

  case WM_COMMAND:
    if (_this)
      return _this->OnWMCommand(wParam, lParam);
    break;

  case WM_TIMER:
    if (_this)
      _this->TimerRoutine((int)wParam);
    break;

  default:
    if (_this == NULL)
      return ::DefWindowProc(hWnd, Msg, wParam, lParam);

    // Is this the task-bar re-creation message?
    if (Msg == m_WM_TASKBARCREATED)
    {
      if (_this->m_bDebug)
        DEBUGP((". taskbar created!\n"));
      _this->showLostIcons();
      break;
    }

    if (_this->m_bDebug)
      DEBUGP(("  hWnd=%08X wm=%08X wParam=%04x lParam=%08X _this=%p\n", hWnd, Msg, wParam, lParam, _this));

    // Is this the callback message used by our tray-icons?
    if (Msg == CALLBACKMESSAGE)
      _this->routeMessageToHandler((int)wParam, (UINT)lParam);

    else
      return _this->OnWindowMessage(hWnd, Msg, wParam, lParam);
  }

  return 0;
}

//-------------------------------------------------------------------------------------
// Shows a given frame in a group
int CTrayIconHandler::groupShow(int grId, int frame)
{
  int err = errOk;

  // Is this group valid?
  trayicongroup_t *tig;
  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  // Given the frame, can we find get the frame-data associated with the frame?
  trayiconframe_t *tif;
  if ((err = trayIconFrameDataFromFrameNumber(*tig, frame, &tif)) != errOk)
    return err;

  // Set active frame
  tig->frame = frame;

  // Update the active frame's icon and tip
  err = updateIcon(tig->iconid, tif->hIcon, _tcslen(tif->szTip) == 0 ? NULL : tif->szTip);
  if (err != errOk)
    return err;

  // If the icon was never shown before then show it
  trayicondata_t *tid;
  findTrayIconDataById(tig->iconid, &tid);

  if (!tid->bVisible)
    err = showIcon(tig->iconid);

  return err;
}

//-------------------------------------------------------------------------------------
// TimerRoutine handler. Given the group-id, it knows what to do next with that given group
// depending on its play-state
void CTrayIconHandler::TimerRoutine(int grId)
{
  trayicongroup_t &tig = m_traygroup[grId];
  if (tig.state == gsPaused)
    return;
  groupFlipIcon(grId);
}

//-------------------------------------------------------------------------------------
// Given a group-id it stops animation then either hides (then delete if needed)
int  CTrayIconHandler::groupHide(int grId, bool bDelete)
{
  trayicongroup_map::iterator it;

  if ((it = m_traygroup.find(grId)) == m_traygroup.end())
    return errNoGroup;

  // Stop animation
  groupAnimate(grId, gsStop, 0);

  trayicongroup_t &tig = it->second;
  
  hideIcon(tig.iconid, bDelete);

  if (bDelete)
    m_traygroup.erase(it);

  return errOk;
}


//-------------------------------------------------------------------------------------
// Updates a given icon (frame) within a group
int CTrayIconHandler::groupUpdateIcon(int grId, UINT frame, HICON hIcon, LPTSTR szTip)
{
  trayicongroup_t *tig;

  int err = errOk;

  if (!findTrayIconGroupById(grId, &tig))
    return errNoGroup;

  trayiconframe_t *tigd;
  if ( (err = trayIconFrameDataFromFrameNumber(*tig, frame, &tigd)) != errOk)
    return err;

  if (szTip == NULL)
    tigd->szTip[0] = '\0';
  else
    _tcscpy(tigd->szTip, szTip);

  if (hIcon != cteKeepIcon)
    tigd->hIcon = hIcon;

  return errOk;
}

void CTrayIconHandler::Wait()
{
  if (!IsStarted())
    return;
  // Wait for thread to terminate itself
  ::WaitForSingleObject(m_hTrayIconThread, INFINITE);
}

//-------------------------------------------------------------------------------------
// Assigns a new window title for the tray-icon hidden window
void CTrayIconHandler::SetWindowName(LPTSTR szWindowName)
{
  size_t n = (sizeof(m_szWindowName)/sizeof(TCHAR)) - 1;

  _tcsncpy(m_szWindowName, szWindowName, n);
  m_szWindowName[n] = 0;
}


#ifdef NIIF_INFO

//-------------------------------------------------------------------------------------
// Shows a ballon-tooltip on the given iconId with the given values
int CTrayIconHandler::showBalloonTip(
                                     int iconId,
                                     LPCTSTR szMsg,
                                     LPCTSTR szTitle, 
                                     UINT uTimeout, 
                                     DWORD dwInfoFlags)
{
  trayicondata_t *tid;
  if (!findTrayIconDataById(iconId, &tid))
    return errNoIcon;

  tid->nid.uFlags      = NIF_INFO;
  tid->nid.uTimeout    = uTimeout;
  tid->nid.dwInfoFlags = dwInfoFlags;

  size_t n;

  n = (sizeof(tid->nid.szInfo) / sizeof(TCHAR)) - 1;
  _tcsncpy(tid->nid.szInfo, szMsg ? szMsg : _T(""), n);
  tid->nid.szInfo[n] = 0;

  n = (sizeof(tid->nid.szInfoTitle) / sizeof(TCHAR)) - 1;
  _tcsncpy(tid->nid.szInfoTitle, szTitle ? szTitle : _T(""), n);
  tid->nid.szInfoTitle[n] = 0;

  ::Shell_NotifyIcon(NIM_MODIFY, &tid->nid);

  return errOk;
}

//-------------------------------------------------------------------------------------
// Sets the focus on the selected trayicon
int CTrayIconHandler::SetIconFocus(int iconId)
{
  trayicondata_t *tid;
  if (!findTrayIconDataById(iconId, &tid))
    return errNoIcon;

  ::Shell_NotifyIcon(NIM_SETFOCUS, &tid->nid);

  return errOk;
}

#endif
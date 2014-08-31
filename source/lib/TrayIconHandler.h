#ifndef __TRAYICONHANDLER_04192004__INC__
#define __TRAYICONHANDLER_04192004__INC__

//#define _WIN32_IE 0x0500
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <shellapi.h>
#include <map>
#include <list>

class CTrayIconHandler
{
protected:
  typedef void (CTrayIconHandler::*IconHandler)(int id, UINT msg);
  typedef void (CTrayIconHandler::*GroupHandler)(int grId, UINT msg, UINT frame);
private:

  TCHAR            m_szWindowName[MAX_PATH];
  int              m_nTrayIconSerial, m_nTrayIconGroupSerial;
  HWND             m_hTrayWindow;
  HANDLE           m_hRaceProtection;

  HANDLE           m_hPollError;

  HANDLE           m_hTrayIconThread;

  HINSTANCE        m_hInstance;

  int              m_nError;

  static UINT      m_WM_TASKBARCREATED;

  // This structure defines a given tray-icon used by this wrapper
  struct trayicondata_t
  {
    int                id;
    NOTIFYICONDATA     nid;
    bool               bVisible;
    IconHandler        handler;
    int                groupid;
  };

  // This struct defines a frame within a group
  // A frame is just what makes a given icon different than the other in a group
  // Basically, the only difference is by the icon itself and its tip text
  struct trayiconframe_t
  {
    TCHAR szTip[256];
    HICON hIcon;
  };

  typedef std::list<trayiconframe_t>      trayiconframe_list;

  // This structure defines a group
  // Basically it holds the base icon-id, group id, and a list of the frames
  struct trayicongroup_t
  {
    int id; // group id
    UINT frame; // active/visible frame
    int state; // animation state
    int delay; // animation delay
    int iconid; // icon id returned by normal addIcon()
    int timerid; // id of animation timer
    trayiconframe_list frames; // hIcons list
    GroupHandler handler; // handler
  };

  typedef std::map<int, trayicondata_t>   trayicondata_map;
  typedef std::map<int, trayicongroup_t>  trayicongroup_map;

  trayicondata_map  m_traymap;
  trayicongroup_map m_traygroup;

  // Register window class and creates the window
  int MakeWindow(bool bUnmake = false);

  static DWORD     WINAPI TrayIconThreadProc(LPVOID Param);
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

  trayicondata_t *findTrayIconDataPtrById(int id);
  bool findTrayIconDataById(int id, trayicondata_t **tid);
  bool findTrayIconGroupById(int grId, trayicongroup_t **tig);
  bool removeTrayIconDataById(int id);
  int  trayIconFrameDataFromFrameNumber(trayicongroup_t &tig, UINT frame, trayiconframe_t **tigd);
  void routeMessageToHandler(int id, UINT msg);
  
  int  hideIcon(trayicondata_t &tid, bool bDelete);

  void TimerRoutine(int grId);

protected:
  bool BeginRaceProtection();
  void EndRaceProtection(bool bTerminate = false);
  bool m_bDebug;

public:
  enum
  {
    // error codes
    errOk = 0,
    errClassRegistration,
    errWindowCreate,
    errThreadProc,
    errNoId,
    errNoIcon,
    errNoGroup,
    errNoFrame,
    errAlreadyPlaying,

    // group animation state/actions
    gsPause, // action: Pause
    gsPaused = gsPause, // state: paused
    gsPlay, // action: play
    gsPlaying = gsPlay, // state: playing
    gsRestart, // action: restart playing->set new delay
    gsStop, // action: stop
    gsStopped = gsStop, // state: not playing

    // some misc constants
    CALLBACKMESSAGE = WM_APP,
    InvalidGroupId = -1
  };

  static const HICON cteKeepIcon;

  CTrayIconHandler(bool bDebug = false);
  virtual ~CTrayIconHandler();

  void SetHInstance(HINSTANCE hInst);
  void SetWindowName(LPTSTR szWindowName);

  inline const bool IsStarted() const { return m_hTrayIconThread != NULL; }

  virtual int Init();
  virtual int Stop();

  // Single icon management routines
  int  addIcon(int &id, HICON hIcon, bool bShow, LPTSTR szTip = NULL, IconHandler handler = NULL);
  int  addIcon(int &id, UINT IconId, bool bShow, LPTSTR szTip = NULL, IconHandler handler = NULL);
  int  addIcon(int &id, LPCTSTR szIcon, bool bShow, LPTSTR szTip = NULL, IconHandler handler = NULL);
  int  updateIcon(const int id, HICON hIcon, LPTSTR szTip = NULL, IconHandler handler = NULL);
  int  showIcon(const int id);
  int  hideIcon(const int id, bool bDelete = false);
  void hideAllIcons(bool bDelete);

  // Utilities
  void showLostIcons();

  static HICON hIconFromFile(LPCTSTR szIcon);
  inline const HWND GetHwnd() const { return m_hTrayWindow; }

  // Group icon management routines
  int  groupCreate(int &grId, GroupHandler handler = 0);
  int  groupGetIconId(int grId);
  int  groupAddIcon(int grId, HICON hIcon, LPTSTR szTip = NULL);
  int  groupAddIcon(int grId, UINT IconId, LPTSTR szTip = NULL);
  int  groupAddIcon(int grId, LPCTSTR szIcon, LPTSTR szTip = NULL);
  int  groupUpdateIcon(int grId, UINT frame, HICON hIcon, LPTSTR szTip = NULL);
  int  groupFlipIcon(int grId);
  int  groupAnimate(int grId, int action, int delay);
  int  groupGetAnimationState(int grId);

  int  groupHide(int grId, bool bDelete = false);
  int  groupShow(int grId, int frame = 0);

  void groupHideAll(bool bDelete);

  static bool  showPopupMenu(HINSTANCE hInst, HWND hWnd, UINT menuId, int menuPos = 0);
  bool         showPopupMenu(UINT menuId, int menuPos = 0);

  virtual LRESULT OnWMCommand(WPARAM wParam, LPARAM lParam) { return 0; }
  virtual LRESULT OnWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { return ::DefWindowProc(hWnd, Msg, wParam, lParam); }

  void Wait();

#ifdef NIIF_INFO
  int showBalloonTip(int iconId, LPCTSTR szMsg, LPCTSTR szTitle, UINT uTimeout, DWORD dwInfoFlags = NIIF_INFO);
  int SetIconFocus(int iconId);
#endif
};


#endif
#include <windows.h>
#include <ida.hpp>
#include <loader.hpp>

#include <kernwin.hpp>
#include <auto.hpp>
#include <netnode.hpp>

#include <idp.hpp>
#include "lib\TrayIconHandler.h"
#include "util.h"
#include "ThingyTray.h"

extern plugin_t PLUGIN;
HINSTANCE g_hInstance;

CThingyTray m_tray;

//--------------------------------------------------------------------------
// This callback is called for notification events
int idaapi notification_callback(void * /*user_data*/, int event_id, va_list /*va*/)
{
  if (!m_tray.IsInTray())
    return 0;

  if (event_id == processor_t::auto_empty_finally)
  {
    m_tray.DoIdle();
  }

  return 0;
}

bool idaapi do_backup_database(void *)
{
  if (!netnode::inited())
    return false;

  char outfile[QMAXPATH];
  SYSTEMTIME st;
  GetSystemTime(&st);

  qsnprintf(outfile, QMAXPATH, "%s_%02d-%02d-%4d-%02d-%02d-%02d.idb", 
    get_filename_no_idb(), st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

  save_database(outfile, false);
  msg("Database backed up to: %s\n", outfile);

  return false;
}

bool idaapi do_minimize_to_tray(void *x)
{
  int v = (int)x;
  m_tray.DoMinimizeToTray();

  if (v)
  {
    ::SetPriorityClass(::GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
  }

  return false;
}

int idaapi init(void)
{
  hook_to_notification_point(HT_IDP, notification_callback, NULL);
  
  PLUGIN.flags &= ~PLUGIN_UNL;

  if (m_tray.Init() != CTrayIconHandler::errOk)
  {
    msg("Failed to initialize tray icon!");
    return PLUGIN_UNL;
  }

  msg("IDA-Thingy v0.2 " __DATE__ " " __TIME__ " - loaded\n");

  add_menu_item("Windows/", "-", "", SETMENU_INS, 0, 0);
  add_menu_item("Windows/", "Minimize to ~t~ray", "", SETMENU_INS, do_minimize_to_tray, 0);
  add_menu_item("Windows/", "Bac~k~ground to tray", "", SETMENU_INS, do_minimize_to_tray, (void *)1);
  add_menu_item("File/Save", "~B~ackup database", "", SETMENU_APP, do_backup_database, 0);

  return (PLUGIN.flags & PLUGIN_UNL) ? PLUGIN_OK : PLUGIN_KEEP;
}

void idaapi term(void)
{
  unhook_from_notification_point(HT_IDP, notification_callback);
  m_tray.Stop();
}

void idaapi run(int arg)
{
#ifdef _DEBUG
  msg("thingy:%d\n", arg);
  if (arg==1)
  {
    m_tray.DoIdle();
  }
  else if (arg==2)
  {
    m_tray.DoIdle(true);
  }
  else if (arg==3)
  {
    m_tray.DoMinimizeToTray();
  }
#else
  if (arg == 1)
  {
    do_minimize_to_tray(0);
  }
  else if (arg == 2)
  {
    do_minimize_to_tray((void *)1);
  }
#endif
}

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_FIX,           // plugin flags
  init,                 // initialize
  term,                 // terminate. this pointer may be NULL.
  run,                  // invoke plugin
  "",              // long comment about the plugin
  "",                 // multiline help about the plugin
  "IDAThingy",          // the preferred short name of the plugin
  ""         // the preferred hotkey to run the plugin
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch(ul_reason_for_call) 
  {
  case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls((HMODULE)hModule);
    g_hInstance = (HINSTANCE) hModule;
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
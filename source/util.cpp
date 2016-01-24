#include "util.h"
#include <ida.hpp>
#include <idp.hpp>
#include <kernwin.hpp>
#include <loader.hpp>

HWND get_ida_hwnd()
{
  return (HWND)callui(ui_get_hwnd).vptr;
}

char *db_status_text()
{
  static char msg[QMAXPATH];

  qsnprintf(msg, QMAXPATH, "'%s' is now busy!", database_idb);

  return msg;
}

char *get_filename_no_idb()
{
  static char path[QMAXPATH];

  qsnprintf(path, QMAXPATH, "%s", database_idb);

  for (int i=strlen(path)-1;i>=0;i--)
  {
    if (path[i] != '.')
      continue;
    path[i] = 0;
    break;
  }
  return path;
}

char *get_idb_path()
{
  static char path[QMAXPATH];

  qsnprintf(path, QMAXPATH, "%s", database_idb);

  for (int i=strlen(path)-1;i>=0;i--)
  {
    if (path[i] != '\\')
      continue;
    path[i+1] = 0;
    break;
  }
  return path;
}
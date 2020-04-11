// Command line switch for restarted application
#ifndef RA_CMDLINE_RESTART_PROCESS
#define RA_CMDLINE_RESTART_PROCESS TEXT("--generate-projects")
#endif

#ifdef _WIN32
// Mutex unique name
#ifndef RA_MUTEX_OTHER_RESTARTING
#define RA_MUTEX_OTHER_RESTARTING TEXT("CCONSTRUCT-RESTART-MUTEX")
#endif

// Return TRUE if Process was restarted
BOOL RA_CheckProcessWasRestarted();

// Check process command line for restart switch
// Call this function to check that is restarted instance
BOOL RA_CheckForRestartProcessStart();

// Wait till previous instance of process finish
BOOL RA_WaitForPreviousProcessFinish();

// Call it when process finish
BOOL RA_DoRestartProcessFinish();

// Call this function when you need restart application
// After call you must close an active instance of your application
BOOL RA_ActivateRestartProcess();

// Global Variables
HANDLE g_RA_hMutexOtherRestarting = NULL;   // Mutex
BOOL g_RA_bWasRestarted           = FALSE;  // Restarted Flag

BOOL RA_CheckProcessWasRestarted() { return g_RA_bWasRestarted; }

BOOL RA_CheckForRestartProcessStart() {
  // Simple find substring in command line
  LPTSTR szCmdLine = GetCommandLine();
  return strstr(szCmdLine, RA_CMDLINE_RESTART_PROCESS) != NULL;
}

BOOL RA_WaitForPreviousProcessFinish() {
  // App restarting
  BOOL AlreadyRunning;
  // Try to Create Mutex
  g_RA_hMutexOtherRestarting = CreateMutex(NULL, FALSE, RA_MUTEX_OTHER_RESTARTING);
  DWORD dwLastError          = GetLastError();
  AlreadyRunning = (dwLastError == ERROR_ALREADY_EXISTS || dwLastError == ERROR_ACCESS_DENIED);
  if (AlreadyRunning) {
    // Waiting for previous instance release mutex
    WaitForSingleObject(g_RA_hMutexOtherRestarting, INFINITE);
    ReleaseMutex(g_RA_hMutexOtherRestarting);
    g_RA_bWasRestarted = TRUE;
  }
  CloseHandle(g_RA_hMutexOtherRestarting);
  g_RA_hMutexOtherRestarting = NULL;
  return TRUE;
}

BOOL RA_DoRestartProcessFinish() {
  // Releasing mutex signal that process finished
  DWORD dwWaitResult = WaitForSingleObject(g_RA_hMutexOtherRestarting, 0);
  if (dwWaitResult == WAIT_TIMEOUT) ReleaseMutex(g_RA_hMutexOtherRestarting);
  CloseHandle(g_RA_hMutexOtherRestarting);
  g_RA_hMutexOtherRestarting = NULL;
  return (dwWaitResult == WAIT_TIMEOUT);
}
BOOL RA_ActivateRestartProcess() {
  // Restart App
  BOOL AlreadyRunning;
  g_RA_hMutexOtherRestarting = CreateMutex(NULL, TRUE, RA_MUTEX_OTHER_RESTARTING);
  DWORD dwLastError          = GetLastError();
  AlreadyRunning = (dwLastError == ERROR_ALREADY_EXISTS || dwLastError == ERROR_ACCESS_DENIED);
  if (AlreadyRunning) {
    WaitForSingleObject(g_RA_hMutexOtherRestarting, INFINITE);
    ReleaseMutex(g_RA_hMutexOtherRestarting);
    CloseHandle(g_RA_hMutexOtherRestarting);
    return FALSE;
  }

  STARTUPINFO si         = {0};
  PROCESS_INFORMATION pi = {0};
  si.cb                  = sizeof(STARTUPINFO);

  // Create New Instance command line
  TCHAR szAppPath[MAX_PATH] = {0};
  GetModuleFileName(NULL, szAppPath, MAX_PATH);
  const char* quotedAppPath = cc_printf("\"%s\" %s", szAppPath, RA_CMDLINE_RESTART_PROCESS);

  // Create another copy of process
  return CreateProcess(NULL, (LPSTR)quotedAppPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}
#endif

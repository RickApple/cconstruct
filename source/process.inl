#ifdef _WIN32
#include <malloc.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#pragma warning(disable : 4996)

typedef struct system_np_s {
  HANDLE child_stdout_read;
  HANDLE child_stderr_read;
  HANDLE reader;
  PROCESS_INFORMATION pi;
  const char* command;
  char* stdout_data;
  int stdout_data_size;
  char* stderr_data;
  int stderr_data_size;
  int* exit_code;
  int timeout;  // timeout in milliseconds or -1 for INIFINTE
} system_np_t;

static int peek_pipe(HANDLE pipe, char* data, int size) {
  char buffer[4 * 1024];
  DWORD read      = 0;
  DWORD available = 0;
  bool b          = PeekNamedPipe(pipe, NULL, sizeof(data), NULL, &available, NULL);
  if (!b) {
    return -1;
  } else if (available > 0) {
    int bytes = min(sizeof(buffer), available);
    b         = ReadFile(pipe, buffer, bytes, &read, NULL);
    if (!b) {
      return -1;
    }
    if (data != NULL && size > 0) {
      int n = min(size - 1, (int)read);
      memcpy(data, buffer, n);
      data[n + 1] = 0;  // always zero terminated
      return n;
    }
  }
  return 0;
}

static DWORD WINAPI read_from_all_pipes_fully(void* p) {
  system_np_t* system             = (system_np_t*)p;
  unsigned long long milliseconds = GetTickCount64();  // since boot time
  char* out =
      system->stdout_data != NULL && system->stdout_data_size > 0 ? system->stdout_data : NULL;
  char* err =
      system->stderr_data != NULL && system->stderr_data_size > 0 ? system->stderr_data : NULL;
  int out_bytes = system->stdout_data != NULL && system->stdout_data_size > 0
                      ? system->stdout_data_size - 1
                      : 0;
  int err_bytes = system->stderr_data != NULL && system->stderr_data_size > 0
                      ? system->stderr_data_size - 1
                      : 0;
  for (;;) {
    int read_stdout = peek_pipe(system->child_stdout_read, out, out_bytes);
    if (read_stdout > 0 && out != NULL) {
      out += read_stdout;
      out_bytes -= read_stdout;
    }
    int read_stderr = peek_pipe(system->child_stderr_read, err, err_bytes);
    if (read_stderr > 0 && err != NULL) {
      err += read_stderr;
      err_bytes -= read_stderr;
    }
    if (read_stdout < 0 && read_stderr < 0) {
      break;
    }  // both pipes are closed
    unsigned long long time_spent_in_milliseconds = GetTickCount64() - milliseconds;
    if (system->timeout > 0 && time_spent_in_milliseconds > system->timeout) {
      break;
    }
    if (read_stdout == 0 && read_stderr == 0) {  // nothing has been read from both pipes
      HANDLE handles[2] = {system->child_stdout_read, system->child_stderr_read};
      WaitForMultipleObjects(2, handles, false,
                             1);  // wait for at least 1 millisecond (more likely 16)
    }
  }
  if (out != NULL) {
    *out = 0;
  }
  if (err != NULL) {
    *err = 0;
  }
  return 0;
}

static int create_child_process(system_np_t* system) {
  SECURITY_ATTRIBUTES sa    = {0};
  sa.nLength                = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle         = true;
  sa.lpSecurityDescriptor   = NULL;
  HANDLE child_stdout_write = INVALID_HANDLE_VALUE;
  HANDLE child_stderr_write = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&system->child_stderr_read, &child_stderr_write, &sa, 0)) {
    return GetLastError();
  }
  if (!SetHandleInformation(system->child_stderr_read, HANDLE_FLAG_INHERIT, 0)) {
    return GetLastError();
  }
  if (!CreatePipe(&system->child_stdout_read, &child_stdout_write, &sa, 0)) {
    return GetLastError();
  }
  if (!SetHandleInformation(system->child_stdout_read, HANDLE_FLAG_INHERIT, 0)) {
    return GetLastError();
  }
  // Set the text I want to run
  STARTUPINFO siStartInfo = {0};
  siStartInfo.cb          = sizeof(STARTUPINFO);
  siStartInfo.hStdError   = child_stderr_write;
  siStartInfo.hStdOutput  = child_stdout_write;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  siStartInfo.wShowWindow = SW_HIDE;

  const char* szCmdline = system->command;

  bool b  = CreateProcessA(NULL, (char*)szCmdline,
                          NULL,              // process security attributes
                          NULL,              // primary thread security attributes
                          true,              // handles are inherited
                          CREATE_NO_WINDOW,  // creation flags
                          NULL,              // use parent's environment
                          NULL,              // use parent's current directory
                          &siStartInfo,      // STARTUPINFO pointer
                          &system->pi);      // receives PROCESS_INFORMATION
  int err = GetLastError();
  CloseHandle(child_stderr_write);
  CloseHandle(child_stdout_write);
  if (!b) {
    CloseHandle(system->child_stdout_read);
    system->child_stdout_read = INVALID_HANDLE_VALUE;
    CloseHandle(system->child_stderr_read);
    system->child_stderr_read = INVALID_HANDLE_VALUE;
  }
  return b ? 0 : err;
}

int system_np(const char* command, int timeout_milliseconds, char* stdout_data,
              int stdout_data_size, char* stderr_data, int stderr_data_size, int* exit_code) {
  system_np_t system = {0};
  if (exit_code != NULL) {
    *exit_code = 0;
  }
  if (stdout_data != NULL && stdout_data_size > 0) {
    stdout_data[0] = 0;
  }
  if (stderr_data != NULL && stderr_data_size > 0) {
    stderr_data[0] = 0;
  }
  system.timeout          = timeout_milliseconds > 0 ? timeout_milliseconds : -1;
  system.command          = command;
  system.stdout_data      = stdout_data;
  system.stderr_data      = stderr_data;
  system.stdout_data_size = stdout_data_size;
  system.stderr_data_size = stderr_data_size;
  int r                   = create_child_process(&system);
  if (r == 0) {
    // auto thr = new std::thread(read_from_all_pipes_fully, &system);
    system.reader = CreateThread(NULL, 0, read_from_all_pipes_fully, &system, 0, NULL);
    {
      bool thread_done  = WaitForSingleObject(system.pi.hThread, timeout_milliseconds) == 0;
      bool process_done = WaitForSingleObject(system.pi.hProcess, timeout_milliseconds) == 0;
      if (!thread_done || !process_done) {
        TerminateProcess(system.pi.hProcess, ETIME);
      }
      if (exit_code != NULL) {
        GetExitCodeProcess(system.pi.hProcess, (DWORD*)exit_code);
      }
      CloseHandle(system.pi.hThread);
      CloseHandle(system.pi.hProcess);
      CloseHandle(system.child_stdout_read);
      system.child_stdout_read = INVALID_HANDLE_VALUE;
      CloseHandle(system.child_stderr_read);
      system.child_stderr_read = INVALID_HANDLE_VALUE;
    }
  }
  if (stdout_data != NULL && stdout_data_size > 0) {
    stdout_data[stdout_data_size - 1] = 0;
  }
  if (stderr_data != NULL && stderr_data_size > 0) {
    stderr_data[stderr_data_size - 1] = 0;
  }
  return r;
}
#endif

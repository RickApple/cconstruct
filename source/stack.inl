#if defined(_WIN32)
void PrintStrackFromContext(PCONTEXT Context) {
  SymSetOptions(SYMOPT_LOAD_LINES);
  SymInitialize(GetCurrentProcess(), NULL, TRUE);

  HANDLE process_handle = GetCurrentProcess();

  DWORD machine_type;
  STACKFRAME64 stack_frame = {0};

  // Set up stack frame.
  #ifdef _M_IX86
  machine_type                 = IMAGE_FILE_MACHINE_I386;
  stack_frame.AddrPC.Offset    = Context->Eip;
  stack_frame.AddrFrame.Offset = Context->Ebp;
  stack_frame.AddrStack.Offset = Context->Esp;
  #elif _M_X64
  machine_type                 = IMAGE_FILE_MACHINE_AMD64;
  stack_frame.AddrPC.Offset    = Context->Rip;
  stack_frame.AddrFrame.Offset = Context->Rsp;
  stack_frame.AddrStack.Offset = Context->Rsp;
  #elif _M_IA64
  machine_type                  = IMAGE_FILE_MACHINE_IA64;
  stack_frame.AddrPC.Offset     = Context->StIIP;
  stack_frame.AddrFrame.Offset  = Context->IntSp;
  stack_frame.AddrBStore.Offset = Context->RsBSP;
  stack_frame.AddrBStore.Mode   = AddrModeFlat;
  stack_frame.AddrStack.Offset  = Context->IntSp;
  #else
    #error "Unsupported platform"
  #endif
  stack_frame.AddrPC.Mode    = AddrModeFlat;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Mode = AddrModeFlat;

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  PSYMBOL_INFO symbol  = (PSYMBOL_INFO)buffer;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen   = MAX_SYM_NAME;

  IMAGEHLP_LINE64 line = {0};
  line.SizeOfStruct    = sizeof(IMAGEHLP_LINE64);

  unsigned frame_count = 0;
  while (true) {
    if (!StackWalk64(machine_type, GetCurrentProcess(), GetCurrentThread(), &stack_frame,
                     machine_type == IMAGE_FILE_MACHINE_I386 ? NULL : Context, NULL,
                     SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
      // Maybe it failed, maybe we have finished walking the stack.
      break;
    }

    if (stack_frame.AddrPC.Offset != 0) {
      const DWORD64 address  = stack_frame.AddrPC.Offset;
      DWORD64 displacement64 = 0;
      DWORD displacement     = 0;

      if (SymFromAddr(process_handle, address, &displacement64, symbol)) {
        if (SymGetLineFromAddr64(process_handle, address, &displacement, &line)) {
          const bool has_passed_main_function = (strstr(line.FileName, "exe_common.inl") != NULL);
          if (has_passed_main_function) return;

          fprintf(stderr, "%i: %s[%s(%i)]\n", frame_count, symbol->Name, line.FileName,
                  line.LineNumber);
        } else {
          DWORD error = GetLastError();
          fprintf(stderr, "%i: %s   SymGetLineFromAddr64 returned error : %d\n", frame_count,
                  symbol->Name, error);
        }
      } else {
        DWORD error = GetLastError();
        fprintf(stderr, "SymFromAddr returned error : %d\n", error);
      }

    } else {
      // Base reached.
      break;
    }
  }
}
#else
void DoStack() {}
#endif

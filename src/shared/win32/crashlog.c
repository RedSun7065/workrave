/*
 * crashlog.c
 *
 * Copyright (C) 2003 Rob Caelers <robc@krandor.org>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * $Id$
 *
 * Based on Dr. Mingw.
 */

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "crashlog.h"
#include "harpoon/harpoon.h"

static void unwind_stack(FILE *log, HANDLE process, PCONTEXT context);
static void dump_registers(FILE *log, PCONTEXT context);
static void dump_registry(FILE *log, HKEY key, char *name);

static 
DWORD GetModuleBase(DWORD dwAddress)
{
  MEMORY_BASIC_INFORMATION Buffer;
	
  return VirtualQuery((LPCVOID) dwAddress, &Buffer, sizeof(Buffer)) ? (DWORD) Buffer.AllocationBase : 0;
}

static EXCEPTION_DISPOSITION __cdecl 
double_exception_handler(struct _EXCEPTION_RECORD *exception_record, 
                         void *establisher_frame, 
                         struct _CONTEXT *context_record, 
                         void *dispatcher_context)
{
  MessageBox(NULL,
             "Workrave has unexpectedly crashed and failed to create a crash "
             "log. This is serious. Please report this to workrave-devel@sourceforge.net or "
             "file a bugreport at: "
             "http://workrave.org/cgi-bin/bugzilla/enter_bug.cgi. " , "Double exception", MB_OK);

  exit(1);
}
EXCEPTION_DISPOSITION __cdecl 
exception_handler(struct _EXCEPTION_RECORD *exception_record, 
                  void *establisher_frame, 
                  struct _CONTEXT *context_record, 
                  void *dispatcher_context)
{
  char crash_log_name[MAX_PATH];
  char crash_text[1024];
  TCHAR szModule[MAX_PATH];
  HMODULE hModule;

  __try1(double_exception_handler);

  harpoon_unblock_input();

  GetModuleFileName(GetModuleHandle(NULL), crash_log_name, sizeof(crash_log_name));
  // crash_log_name == c:\program files\workrave\lib\workrave.exe
  char *s = strrchr(crash_log_name, '\\');
  assert (s);
  *s = '\0';
  // crash_log_name == c:\program files\workrave\lib
  s = strrchr(crash_log_name, '\\');
  assert (s);
  *s = '\0';
  // crash_log_name == c:\program files\workrave
  strcat(crash_log_name, "\\workrave.log");
  
  FILE *log = fopen(crash_log_name, "w");
  if (log != NULL)
    {
      SYSTEMTIME SystemTime;

      GetLocalTime(&SystemTime);	
      fprintf(log, "Crash log created on %02d/%02d/%04d at %02d:%02d:%02d.\n\n",
              SystemTime.wDay,
              SystemTime.wMonth,
              SystemTime.wYear,
              SystemTime.wHour,
              SystemTime.wMinute,
              SystemTime.wSecond);
      
      fprintf(log, "code = %x\n", exception_record->ExceptionCode);
      fprintf(log, "flags = %x\n", exception_record->ExceptionFlags);
      fprintf(log, "address = %x\n", exception_record->ExceptionAddress);
      fprintf(log, "params = %d\n", exception_record->NumberParameters);

      fprintf(log, "%s caused ",  GetModuleFileName(NULL, szModule, MAX_PATH) ? szModule : "Application");
      switch (exception_record->ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
          fprintf(log, "an Access Violation");
          break;
	
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
          fprintf(log, "an Array Bound Exceeded");
          break;
	
        case EXCEPTION_BREAKPOINT:
          fprintf(log, "a Breakpoint");
          break;
	
        case EXCEPTION_DATATYPE_MISALIGNMENT:
          fprintf(log, "a Datatype Misalignment");
          break;
	
        case EXCEPTION_FLT_DENORMAL_OPERAND:
          fprintf(log, "a Float Denormal Operand");
          break;
	
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
          fprintf(log, "a Float Divide By Zero");
          break;
	
        case EXCEPTION_FLT_INEXACT_RESULT:
          fprintf(log, "a Float Inexact Result");
          break;
	
        case EXCEPTION_FLT_INVALID_OPERATION:
          fprintf(log, "a Float Invalid Operation");
          break;
	
        case EXCEPTION_FLT_OVERFLOW:
          fprintf(log, "a Float Overflow");
          break;
	
        case EXCEPTION_FLT_STACK_CHECK:
          fprintf(log, "a Float Stack Check");
          break;
	
        case EXCEPTION_FLT_UNDERFLOW:
          fprintf(log, "a Float Underflow");
          break;
	
        case EXCEPTION_GUARD_PAGE:
          fprintf(log, "a Guard Page");
          break;

        case EXCEPTION_ILLEGAL_INSTRUCTION:
          fprintf(log, "an Illegal Instruction");
          break;
	
        case EXCEPTION_IN_PAGE_ERROR:
          fprintf(log, "an In Page Error");
          break;
	
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
          fprintf(log, "an Integer Divide By Zero");
          break;
	
        case EXCEPTION_INT_OVERFLOW:
          fprintf(log, "an Integer Overflow");
          break;
	
        case EXCEPTION_INVALID_DISPOSITION:
          fprintf(log, "an Invalid Disposition");
          break;
	
        case EXCEPTION_INVALID_HANDLE:
          fprintf(log, "an Invalid Handle");
          break;

        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
          fprintf(log, "a Noncontinuable Exception");
          break;

        case EXCEPTION_PRIV_INSTRUCTION:
          fprintf(log, "a Privileged Instruction");
          break;
	
        case EXCEPTION_SINGLE_STEP:
          fprintf(log, "a Single Step");
          break;
	
        case EXCEPTION_STACK_OVERFLOW:
          fprintf(log, "a Stack Overflow");
          break;

        case DBG_CONTROL_C:
          fprintf(log, "a Control+C");
          break;
	
        case DBG_CONTROL_BREAK:
          fprintf(log, "a Control+Break");
          break;
	
        case DBG_TERMINATE_THREAD:
          fprintf(log, "a Terminate Thread");
          break;
	
        case DBG_TERMINATE_PROCESS:
          fprintf(log, "a Terminate Process");
          break;
	
        case RPC_S_UNKNOWN_IF:
          fprintf(log, "an Unknown Interface");
          break;
	
        case RPC_S_SERVER_UNAVAILABLE:
          fprintf(log, "a Server Unavailable");
          break;
	
        default:
          fprintf(log, "an Unknown [0x%lX] Exception", exception_record->ExceptionCode);
          break;
        }

      fprintf(log, " at location %08x", (DWORD) exception_record->ExceptionAddress);
      if ((hModule = (HMODULE) GetModuleBase((DWORD) exception_record->ExceptionAddress) && GetModuleFileName(hModule, szModule, sizeof(szModule))))
        fprintf(log, " in module %s", szModule);
	
      // If the exception was an access violation, print out some additional information, to the error log and the debugger.
      if(exception_record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && exception_record->NumberParameters >= 2)
        fprintf(log, " %s location %08x\n\n", exception_record->ExceptionInformation[0] ? "writing to" : "reading from", exception_record->ExceptionInformation[1]);
  
      DWORD pid = GetCurrentProcessId();
      HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

      dump_registers(log, context_record);
      unwind_stack(log, process, context_record);
      fprintf(log, "\nRegistry dump:\n\n");
      dump_registry(log, HKEY_CURRENT_USER, "Software\\Workrave");
      
      fclose(log);
    }

  snprintf(crash_text, 1023,
           "Workrave has unexpectedly crashed. A crash log has been saved to "
           "%s. Please mail this file to workrave-devel@sourceforge.net or "
           "file a bugreport at: "
           "http://workrave.org/cgi-bin/bugzilla/enter_bug.cgi. "
           "Thanks.", crash_log_name);
  
  MessageBox(NULL, crash_text, "Exception", MB_OK);

  __except1;
  
  exit(1);
}


static
BOOL WINAPI stack_walk(HANDLE process, LPSTACKFRAME stack_frame, PCONTEXT context_record)
{
  if (!stack_frame->Reserved[0])
    {
      stack_frame->Reserved[0] = 1;
		
      stack_frame->AddrPC.Mode = AddrModeFlat;
      stack_frame->AddrPC.Offset = context_record->Eip;
      stack_frame->AddrStack.Mode = AddrModeFlat;
      stack_frame->AddrStack.Offset = context_record->Esp;
      stack_frame->AddrFrame.Mode = AddrModeFlat;
      stack_frame->AddrFrame.Offset = context_record->Ebp;
      
      stack_frame->AddrReturn.Mode = AddrModeFlat;
      if (!ReadProcessMemory(process,
                             (LPCVOID) (stack_frame->AddrFrame.Offset + sizeof(DWORD)),
                             &stack_frame->AddrReturn.Offset, sizeof(DWORD), NULL))
        return FALSE;
    }
  else
    {
      stack_frame->AddrPC.Offset = stack_frame->AddrReturn.Offset;

      if (!ReadProcessMemory(process, (LPCVOID) stack_frame->AddrFrame.Offset,
                            &stack_frame->AddrFrame.Offset, sizeof(DWORD), NULL))
        return FALSE;
                
      if (!ReadProcessMemory(process, (LPCVOID) (stack_frame->AddrFrame.Offset + sizeof(DWORD)),
                             &stack_frame->AddrReturn.Offset, sizeof(DWORD), NULL))
        return FALSE;
    }

  ReadProcessMemory(process, (LPCVOID) (stack_frame->AddrFrame.Offset + 2*sizeof(DWORD)),
                    stack_frame->Params, sizeof(stack_frame->Params), NULL);
	
  return TRUE;	
}

static void
unwind_stack(FILE *log, HANDLE process, PCONTEXT context)
{
  STACKFRAME          sf;

  fprintf(log, "Stack trace:\n\n");

  ZeroMemory(&sf,  sizeof(STACKFRAME));
  sf.AddrPC.Offset    = context->Eip;
  sf.AddrPC.Mode      = AddrModeFlat;
  sf.AddrStack.Offset = context->Esp;
  sf.AddrStack.Mode   = AddrModeFlat;
  sf.AddrFrame.Offset = context->Ebp;
  sf.AddrFrame.Mode   = AddrModeFlat;

  fprintf(log, "PC        Frame     Ret\n");
  
  while (TRUE)
    {
      if (!stack_walk(process, &sf, context))
        break;
     
      if (sf.AddrFrame.Offset == 0)
        break;
     
      fprintf(log, "%08X  %08X  %08X\n",  
              sf.AddrPC.Offset,
              sf.AddrFrame.Offset,
              sf.AddrReturn.Offset);
    }
}

static void
dump_registers(FILE *log, PCONTEXT context)
{
  fprintf(log, "Registers:\n\n");

  if (context->ContextFlags & CONTEXT_INTEGER)
    {
      fprintf(log, "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
              context->Eax, context->Ebx, context->Ecx, context->Edx,
              context->Esi, context->Edi);
    }
  
  if (context->ContextFlags & CONTEXT_CONTROL)
    {
      fprintf(log, "eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx %s %s %s %s %s %s %s %s %s %s\n",
              context->Eip, context->Esp, context->Ebp,
              (context->EFlags >> 12) & 3,	//  IOPL level value
              context->EFlags & 0x00100000 ? "vip" : "   ",	//  VIP (virtual interrupt pending)
              context->EFlags & 0x00080000 ? "vif" : "   ",	//  VIF (virtual interrupt flag)
              context->EFlags & 0x00000800 ? "ov" : "nv",	//  VIF (virtual interrupt flag)
              context->EFlags & 0x00000400 ? "dn" : "up",	//  OF (overflow flag)
              context->EFlags & 0x00000200 ? "ei" : "di",	//  IF (interrupt enable flag)
              context->EFlags & 0x00000080 ? "ng" : "pl",	//  SF (sign flag)
              context->EFlags & 0x00000040 ? "zr" : "nz",	//  ZF (zero flag)
              context->EFlags & 0x00000010 ? "ac" : "na",	//  AF (aux carry flag)
              context->EFlags & 0x00000004 ? "po" : "pe",	//  PF (parity flag)
              context->EFlags & 0x00000001 ? "cy" : "nc"	//  CF (carry flag)
              );
    }
  
  if (context->ContextFlags & CONTEXT_SEGMENTS)
    {
      fprintf(log, "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  gs=%04lx",
              context->SegCs, context->SegSs, context->SegDs, context->SegEs,
              context->SegFs, context->SegGs);

      if(context->ContextFlags & CONTEXT_CONTROL)
        {
          fprintf(log, "             efl=%08lx", context->EFlags);
        }
    }
  else
    {
      if (context->ContextFlags & CONTEXT_CONTROL)
        {
          fprintf(log, "                                                                       efl=%08lx",
                  context->EFlags);
        }
    }
  
  fprintf(log, "\n\n");
}

static void
save_key(FILE *log, HKEY key, char *name)
{
  DWORD i;
  char keyname[512];
  int keyname_len = strlen(keyname);

  fprintf(log, "key = %s\n", name);
  
  for (i = 0; ; i++)
    {
      char val[256];
      DWORD val_size = sizeof(val);
      BYTE data[0x4000];
      DWORD data_size = sizeof(data);
      DWORD type;
  
      LONG rc = RegEnumValue(key, i, val, &val_size, 0, &type, data, &data_size);

      if (rc != ERROR_SUCCESS)
        break;

      if (val_size)
        fprintf(log, "  value = %s\n", val);

      if (type == REG_SZ)
        {
          fprintf(log, "  string data = %s\n", data);
        }
      else if (type == REG_DWORD && data_size==4)
        {
          fprintf(log, "  dword data = %08lx\n", data);
        }
      else
        {
          fprintf(log, "  hex data = [unsupported]\n");
        }
    }

  fprintf(log, "\n");

  strcpy(keyname, name);
  strcat(keyname, "\\");
  keyname_len = strlen(keyname);
  
  for (i = 0; ; i++)
    {
      HKEY subkey;
      LONG rc = RegEnumKey(key, i, keyname + keyname_len,
                           sizeof(keyname) - keyname_len);

      if (rc != ERROR_SUCCESS)
        break;

      rc = RegOpenKey(key, keyname + keyname_len, &subkey);
      if (rc == ERROR_SUCCESS)
        {
          dump_registry(log, subkey, keyname);
          RegCloseKey(subkey);
        }
    }
}


static void
dump_registry(FILE *log, HKEY key, char *name)
{
  HKEY handle;
  LONG rc = RegOpenKeyEx(HKEY_CURRENT_USER, name, 0, KEY_ALL_ACCESS, &handle);

  if (rc == ERROR_SUCCESS)
    {
      save_key(log, handle, name);
      RegCloseKey(handle);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "addr2line.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Addr2Line::Addr2Line (const std::string &addr2lineTool, const std::vector <std::string> *sysroots)
  : m_addr2lineTool (addr2lineTool)
  , m_sysroots (sysroots)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Addr2Line::~Addr2Line ()
{
  auto it = m_instances.begin ();

  while (it != m_instances.end ())
  {
    DestroyChildProcess (it->first, &it->second);

    it++;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Addr2Line::Symbolicate (const std::string &lib, const std::string &address, std::string *symbol)
{
  // 
  // Create a new instance of a Addr2Line if this library hasn't already spawned a process.
  // 

  std::string stdOutBuffer, stdErrBuffer;

  if (m_instances.empty () || (m_instances.find (lib) == m_instances.end ()))
  {
    Addr2LineProcess childProcess;

    memset (&childProcess, 0, sizeof (Addr2LineProcess));

    if (!CreateChildProcess (lib, &childProcess))
    {
      return false;
    }

    if (ReadFromChildProcess (&childProcess, &stdOutBuffer, &stdErrBuffer) && (!stdErrBuffer.empty ()))
    {
      // Skip processing if any stderr output is processed.

      DestroyChildProcess (lib, &childProcess);

      return false;
    }
  }

  auto it = m_instances.find (lib);

  if (it != m_instances.end ())
  {
    char writeBuffer [BUFSIZ];

    memset (writeBuffer, 0, sizeof (writeBuffer));

    // 
    // Input the raw library offset address (minus any leading hex-specifier).
    // 

    if (address.substr (0, 2) == "0x")
    {
      sprintf (writeBuffer, "%s\r\n", address.substr (2).c_str ());
    }
    else
    {
      sprintf (writeBuffer, "%s\r\n", address.c_str ());
    }

    const size_t writeSize = strlen (writeBuffer);

    bool hasInput = WriteToChildProcess (&it->second, writeBuffer, writeSize);

    if (!hasInput)
    {
      return false;
    }

    // 
    // Parse output which should be in following format:
    // 
    //   0x000ABCDEF\r\n
    //   FunctionPrototype\r\n
    //   source:line
    // 

    bool hasOutput = false;

    while (!hasOutput)
    {
      hasOutput = ReadFromChildProcess (&it->second, &stdOutBuffer, &stdErrBuffer);

      if (!stdErrBuffer.empty ())
      {
        // Skip processing if any stderr output is processed.

        DestroyChildProcess (lib, &it->second);

        return false;
      }
      else if (!stdOutBuffer.empty ())
      {
        size_t lineStart = 0;

        size_t lineEnd = stdOutBuffer.find ('\n');

        while (lineEnd != std::string::npos)
        {
          std::string line = stdOutBuffer.substr (lineStart, lineEnd - lineStart);

          if (!line.empty () && symbol)
          {
            *symbol = line;

            return true;
          }

          lineStart = lineEnd;

          lineEnd = stdOutBuffer.find ('\n', lineEnd + 1);
        }
      }
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Addr2Line::CreateChildProcess (const std::string &lib, Addr2LineProcess *process)
{
  bool success = false;

#ifdef WIN32

  SECURITY_ATTRIBUTES sa;

  sa.nLength = sizeof (sa);

  sa.lpSecurityDescriptor = NULL;

  sa.bInheritHandle = TRUE;

  // 
  // Create a pipe for the child process's STDOUT.
  // 

  if (!CreatePipe (&process->pipes [StdOutRead], &process->pipes [StdOutWrite], &sa, 0))
  { 
    return false;
  }

#if 1
  if (!CreatePipe (&process->pipes [StdErrRead], &process->pipes [StdErrWrite], &sa, 0))
  {
    return false;
  }
#endif

#if 1
  if (!CreatePipe (&process->pipes [StdInRead], &process->pipes [StdInWrite], &sa, 0))
  {
    return false;
  }
#endif

  // 
  // Ensure the handles the parent will use aren't inherited.
  // 

  if (process->pipes [StdOutRead])
  {
    HANDLE stdOutReadTmp = process->pipes [StdOutRead];

    bool success = DuplicateHandle (GetCurrentProcess (), stdOutReadTmp, GetCurrentProcess (), &process->pipes [StdOutRead], 0, TRUE, DUPLICATE_SAME_ACCESS);

    _ASSERT (success);

    success = CloseHandle (stdOutReadTmp);

    _ASSERT (success);
  }

  if (process->pipes [StdErrRead])
  {
    HANDLE stdErrReadTmp = process->pipes [StdErrRead];

    bool success = DuplicateHandle (GetCurrentProcess (), stdErrReadTmp, GetCurrentProcess (), &process->pipes [StdErrRead], 0, TRUE, DUPLICATE_SAME_ACCESS);

    _ASSERT (success);

    success = CloseHandle (stdErrReadTmp);

    _ASSERT (success);
  }

  if (process->pipes [StdInWrite])
  {
    HANDLE stdInWriteTmp = process->pipes [StdInWrite];

    bool success = DuplicateHandle (GetCurrentProcess (), stdInWriteTmp, GetCurrentProcess (), &process->pipes [StdInWrite], 0, TRUE, DUPLICATE_SAME_ACCESS);

    _ASSERT (success);

    success = CloseHandle (stdInWriteTmp);

    _ASSERT (success);
  }

  // 
  // Attempt to find the target library, from the provided sysroots.
  // 

  char cmdline [BUFSIZ] = "";

  for (int i = 0; i < m_sysroots->size (); ++i)
  {
    char path [BUFSIZ];

    const std::string &sysroot = m_sysroots->at (i);

    sprintf (path, "%s%s", sysroot.c_str (), lib.c_str ());

    DWORD attribs = GetFileAttributes (path);

    if ((attribs != INVALID_FILE_ATTRIBUTES) && !(attribs & FILE_ATTRIBUTE_DIRECTORY))
    {
      sprintf (cmdline, "\"%s\" --addresses --demangle --functions --inlines --pretty-print --exe=\"%s\" ", m_addr2lineTool.c_str (), path);

      break;
    }
  }

  if (!strlen (cmdline))
  {
    sprintf (cmdline, "\"%s\" --addresses --demangle --functions --inlines --pretty-print --exe=\"%s\" ", m_addr2lineTool.c_str (), lib.c_str ());
  }

  // 
  // Set up members of the STARTUPINFO structure. 
  // This structure specifies the STDERR and STDOUT handles for redirection.
  // 

  STARTUPINFO startInfo;

  memset (&startInfo, 0, sizeof (STARTUPINFO));

  startInfo.cb = sizeof (STARTUPINFO);

  startInfo.hStdOutput = process->pipes [StdOutWrite];

  startInfo.hStdError = (process->pipes [StdErrWrite] != 0) ? process->pipes [StdErrWrite] : process->pipes [StdOutWrite];

  startInfo.hStdInput = process->pipes [StdInRead];

  startInfo.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION processInfo;

  memset (&processInfo, 0, sizeof (PROCESS_INFORMATION));

  // 
  // Create the child process.
  // 

  success = CreateProcess (NULL,
    cmdline, // command line 
    &sa, // process security attributes 
    NULL, // primary thread security attributes 
    TRUE, // handles are inherited 
    0, //CREATE_NEW_CONSOLE, // creation flags 
    NULL, // use parent's environment 
    NULL, // use parent's current directory 
    &startInfo, // STARTUPINFO pointer 
    &processInfo); // receives PROCESS_INFORMATION

  if (success)
  {

    // 
    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    // 

    /*CloseHandle (startInfo.hStdOutput);

    process->pipes [StdOutWrite] = 0;

    if (startInfo.hStdError != startInfo.hStdOutput)
    {
      CloseHandle (startInfo.hStdError);

      process->pipes [StdErrWrite] = 0;
    }

    CloseHandle (startInfo.hStdInput);

    process->pipes [StdInRead] = 0;*/

    // 
    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example. 
    // 

    process->processInfo = processInfo;

    std::pair <std::string, Addr2LineProcess> entry (lib, *process);

    m_instances.insert (entry);

#if _DEBUG

    fprintf (stdout, "[pid:%d] %s\n", process->processInfo.dwProcessId, cmdline);

    fflush (stdout);

#endif
  }

#endif

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Addr2Line::DestroyChildProcess (const std::string &lib, Addr2LineProcess *process)
{
  bool success = false;

#ifdef WIN32

  for (int i = 0; i < NumPipeTypes; ++i)
  {
    if (process->pipes [i])
    {
      success = CloseHandle (process->pipes [i]);

      _ASSERT (success);

      process->pipes [i] = 0;
    }
  }

  if (process->processInfo.hProcess)
  {
    success = CloseHandle (process->processInfo.hProcess);

    _ASSERT (success);

    process->processInfo.hProcess = 0;
  }

  if (process->processInfo.hThread)
  {
    success = CloseHandle (process->processInfo.hThread);

    _ASSERT (success);

    process->processInfo.hThread = 0;
  }

#endif

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Addr2Line::WriteToChildProcess (Addr2LineProcess *process, char *buffer, size_t bytesToWrite)
{
  unsigned long bytesWritten;

  bool success = false;

#ifdef WIN32

  while (process->pipes [StdInWrite] != 0)
  {
    success = WriteFile (process->pipes [StdInWrite], buffer, bytesToWrite, &bytesWritten, NULL);

    if (!success)
    {
      _ASSERT (success);

      break;
    }
    else if (bytesWritten > 0)
    {
#if _DEBUG

      fprintf (stdout, "[pid:%d][stdin] %s\n", process->processInfo.dwProcessId, buffer);

      fflush (stdout);

#endif

      break;
    }
  }

#endif

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Addr2Line::ReadFromChildProcess (Addr2LineProcess *process, std::string *stdoutBuffer, std::string *stderrBuffer)
{
  bool stdOutSuccess = false, stdErrSuccess = false;

  // 
  // StdOut parsing.
  // 

#ifdef WIN32

  char buffer [512];

  DWORD bytesRead = 0, bytesAvailable = 0;

  while (process->pipes [StdOutRead] != 0)
  {
    bool success = PeekNamedPipe (process->pipes [StdOutRead], NULL, 0, NULL, &bytesAvailable, NULL);

    if (!success || bytesAvailable == 0)
    {
      break;
    }

    success = ReadFile (process->pipes [StdOutRead], buffer, sizeof (buffer), &bytesRead, NULL);

    if (!success)
    {
      break;
    }

    stdOutSuccess = true;

    if (bytesRead > 0 && stdoutBuffer)
    {
      std::string output (buffer, bytesRead);

#if _DEBUG

      fprintf (stdout, "[pid:%d][stdout] %s\n", process->processInfo.dwProcessId, output.c_str ());

      fflush (stdout);

#endif

      *stdoutBuffer += output;
    }
  }

  // 
  // StdErr parsing.
  // 

  bytesRead = 0;

  while (process->pipes [StdErrRead] != 0)
  {
    bool success = PeekNamedPipe (process->pipes [StdErrRead], NULL, 0, NULL, &bytesAvailable, NULL);

    if (!success || bytesAvailable == 0)
    {
      break;
    }

    success = ReadFile (process->pipes [StdErrRead], buffer, sizeof (buffer), &bytesRead, NULL);

    if (!success)
    {
      break;
    }

    stdErrSuccess = true;

    if (bytesRead > 0 && stderrBuffer)
    {
      std::string output (buffer, bytesRead);

#if _DEBUG

      fprintf (stderr, "[pid:%d][stderr] %s\n", process->processInfo.dwProcessId, output.c_str ());

      fflush (stderr);

#endif

      *stderrBuffer += output;
    }
  }

#endif

  return (stdOutSuccess || stdErrSuccess);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

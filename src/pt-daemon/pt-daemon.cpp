////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://github.com/oliver/ptrace-sampler/blob/master/ptrace-sampler.C#L701

// https://github.com/dicej/profile/blob/master/src/profile.cpp

// http://stackoverflow.com/questions/18577956/how-to-use-ptrace-to-get-a-consistent-view-of-multiple-threads

// https://github.com/cgjones/android-system-core/blob/master/debuggerd/debuggerd.c

// http://code.metager.de/source/xref/android/4.4/art/runtime/utils.cc (DumpNativeStack)

// https://bitbucket.org/soko/mozilla-central/src/3670d1eb1b31720069f1f25cb6203ec7078daa7e/tools/profiler/?at=default

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmap_manager.h"

#include "stack_corkscrew.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstring>

#include <stdlib.h>

#include <stdio.h>

#include <stdint.h>

#include <errno.h>

#include <set>

#include <signal.h>

#include <time.h>

#include <unordered_map>

#include <vector>

#include <sys/types.h>

#if defined (__ANDROID__)

#include <dirent.h>

#include <unistd.h>

#include <sys/wait.h>

#include <sys/ptrace.h>

#include <sys/syscall.h>

#include <libunwind.h>

#include <libunwind-ptrace.h>

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ERR_FAILED 1L

#define ERR_INVALID_ARG 2L

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pid_t g_pid = 0;

std::vector <pid_t> g_tids;

uint32_t g_sampleFrequencyHz = 1000L;

volatile bool g_terminate = false;

bool g_verbose = false;

FILE *g_frameDataFile = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int64_t GetCurrentTimeNanos ()
{
#if defined (ANDROID)

  struct timespec now = { 0, 0 };

  if (clock_gettime (CLOCK_MONOTONIC, &now) != 0)
  {
    printf ("Failed to evaluate current time (%d). %s.\n", CLOCK_MONOTONIC, strerror (errno));
  }

  return (now.tv_sec * 1000000000LL) + now.tv_nsec;

#else

  return 0;

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool GetProcessesThreads (pid_t pid, std::vector<pid_t> &processThreads)
{
  // 
  // Acquire a list of running child processes from this ppid (parent pid).
  // On Linux these can either be spawned processes, or threads.
  // 

#if defined (ANDROID)

  char procPath [256];

  sprintf (procPath, "/proc/%d/task/", pid);

  DIR *procRoot = opendir (procPath);

  if (!procRoot)
  {
    return false;
  }

  struct dirent *entry = readdir (procRoot);

  while (entry)
  {
    if (entry->d_name [0] != '.') // Ignore canonical (.) and relative (..) entries.
    {
      char* end;

      const pid_t pid = (pid_t) strtoul(entry->d_name, &end, 10);

      if (*end == '\0')
      {
        processThreads.push_back (pid);
      }
    }

    entry = readdir (procRoot);
  }

  closedir (procRoot);

  return true;

#else

  return false;

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined (ANDROID)

static inline int tgkill (pid_t tgid, pid_t tid, int sig)
{
  // 
  // A wrapper for the tgkill syscall: send a signal to a specific thread.
  // 

  return syscall (__NR_tgkill, tgid, tid, sig);
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined (ANDROID)

static void SignalHandler (int sig)
{
  // 
  // Logging in this re-entrant function will probably cause a deadlock.
  // 

  (void) sig;

  if (sig != SIGCHLD)
  {
    g_terminate = true;
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined (ANDROID)

static long PtraceAttach (pid_t pid)
{
  long result;

  errno = 0;

  while (true)
  {
    result = ptrace (PTRACE_ATTACH, pid, NULL, NULL);

    if ((result != -0) && (errno == EBUSY || errno == EFAULT || errno == ESRCH))
    {
      sched_yield ();

      continue;
    }

    break;
  }

  if ((result == -1L) || (errno != 0))
  {
    fprintf (stderr, "Failed to attach to PID %d. %s.\n", pid, strerror (errno));

    fflush (stderr);
  }

  return result;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined (ANDROID)

static long PtraceDetach (pid_t pid)
{
  long result;

  errno = 0;

  while (true)
  {
    result = ptrace (PTRACE_DETACH, pid, NULL, NULL);

    if ((result != -0) && (errno == EBUSY || errno == EFAULT || errno == ESRCH))
    {
      sched_yield ();

      continue;
    }

    break;
  }

  if ((result == -1L) || (errno != 0))
  {
    fprintf (stderr, "Failed to detach to PID %d. %s.\n", pid, strerror (errno));

    fflush (stderr);
  }

  return result;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void PrintUsage ()
{
  fprintf (stdout, "Usage: --pid <pid> [--tid <tid>]+ [--frequency <hz>] [--verbose]\n\
  --pid\t\tPID of the target process.\n\
  --tid\t\tTID of one or more target threads. This can occur multiple times.\n\
  --frequency\tNumber of samples per-second (default: 1000).\n\
  --verbose\n");

  fflush (stdout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
  // 
  // Process command-line arguments.
  // 
  
  for (int i = 1; i < argc; ++i)
  {
    if ((strcmp (argv [i], "--pid") == 0) && (i < (argc - 1)))
    {
      char *end;

      g_pid = strtoul (argv [++i], &end, 10);
    }
    else if ((strcmp (argv [i], "--tid") == 0) && (i < (argc - 1)))
    {
      char *end;

      pid_t tid = strtoul (argv [++i], &end, 10);

      g_tids.push_back (tid);
    }
    else if ((strcmp (argv [i], "--frequency") == 0) && (i < (argc - 1)))
    {
      char *end;

      g_sampleFrequencyHz = strtoul (argv [++i], &end, 10);
    }
    else if ((strcmp (argv [i], "--verbose") == 0))
    {
      g_verbose = true;
    }
    else
    {
      fprintf (stderr, "Unexpected argument: '%s'\n", argv [i]);

      fflush (stderr);
    }
  }

  // 
  // Validate required arguments were actually specified.
  // 

  if (g_pid == 0)
  {
    fprintf (stderr, "--pid was expected but not specified.\n");

    fflush (stderr);

    PrintUsage ();

    return ERR_INVALID_ARG;
  }

  // 
  // Setup debug logging and signal handlers.
  // 

#if defined (ANDROID)

  setvbuf (stdout, NULL, _IONBF, 0);

  signal (SIGINT, SignalHandler);

  signal (SIGQUIT, SignalHandler);

  signal (SIGTERM, SignalHandler);

  signal (SIGCHLD, SignalHandler);

#endif

  // 
  // Open file for JSON frame logging.
  // 

  char jsonDataFilename [256];

#if defined (ANDROID)

  sprintf (jsonDataFilename, "/data/local/tmp/cpuprofiler.json");

  g_frameDataFile = fopen (jsonDataFilename, "w");

#endif

  if (!g_frameDataFile)
  {
    sprintf (jsonDataFilename, "cpuprofiler.json");

    g_frameDataFile = fopen (jsonDataFilename, "w");
  }

  if (!g_frameDataFile)
  {
    fprintf (stderr, "Failed to open exportable frame data file (%s). %s.\n", jsonDataFilename, strerror (errno));

    fflush (stderr);

    errno = 0;

    return ERR_FAILED;
  }

  fprintf (g_frameDataFile, "{");

  // 
  // Read and report the current shared library mappings.
  // (we'll also include .dex and .oat files, as these might actually be useful later)
  // 

  MemoryMapManager mmapManager;

  {
    char filename [64];

    snprintf (filename, 64, "/proc/%d/maps", g_pid);

    if (mmapManager.ParseUnixProcessMapsFile (filename))
    {
      std::string json;

      mmapManager.ConvertToJSON (json);

      fprintf (g_frameDataFile, json.c_str ());

      fprintf (g_frameDataFile, ","); // trailing comma.
    }
  }

  // 
  // Gather data on any of the current process threads. Regardless of whether selected for profiling.
  // 

  {
    std::vector <pid_t> processThreads;

    if (!GetProcessesThreads (g_pid, processThreads))
    {
      fprintf (stderr, "Failed retrieving threads for PID (%d).\n", g_pid);

      fflush (stderr);

      return ERR_FAILED;
    }

    fprintf (g_frameDataFile, "\"threads\":[");

    for (uint32_t i = 0; i < processThreads.size (); ++i)
    {
      const pid_t thread = processThreads [i];

      fprintf (g_frameDataFile, "%s{\"tid\":%ld,\"name\":\"Thread %ld\"}", ((i > 0) ? "," : ""), thread, thread);
    }

    fprintf (g_frameDataFile, "],");
  }

  if (g_verbose)
  {
    printf ("Attaching to parent PID (%d)...\n", g_pid);
  }

  fprintf (g_frameDataFile, "\"samples\":[");

  // 
  // Scheduled timer wait loop.
  // 

  const int64_t samplingInterval = (1000000000LL / g_sampleFrequencyHz);

  int64_t lastUpdateTimestampNanos = GetCurrentTimeNanos () - samplingInterval;

  while (!g_terminate)
  {
    const int64_t deltaSinceLastSampleNanos = (GetCurrentTimeNanos () - lastUpdateTimestampNanos);

    const bool elapsed = (deltaSinceLastSampleNanos >= samplingInterval);

    if (elapsed)
    {
      // 
      // Validate suspended status of the target process.
      // 

      bool suspended = false;

    #if defined (ANDROID) && 0

      {
        errno = 0;

        int status, result;

        while (true)
        {
          result = waitpid (g_pid, &status, WUNTRACED | WCONTINUED);

          if ((result == (pid_t) -1) && (errno == EINTR))
          {
            sched_yield ();

            continue;
          }
          else if (errno == ECHILD)
          {
            // The parent wasn't forked, so we don't mind.

            result = 0;

            errno = 0;
          }

          break;
        }

        if ((result != 0) || (errno != 0))
        {
          fprintf (stderr, "Failed waiting on PID (%d). %s.\n", g_pid, strerror (errno));

          fflush (stderr);

          return ERR_FAILED;
        }
        else if (WIFEXITED (status) || WIFSIGNALED (status))
        {
          fprintf (stderr, "Target PID (%d) has terminated; aborting.\n", g_pid);

          fprintf (stderr);

          return ERR_FAILED;
        }
        else if (WIFSTOPPED (status))
        {
          const int signalCode = WSTOPSIG (status);

          suspended = true;

          if (g_verbose)
          {
            printf ("Target PID (%u) suspended with signal %d (%s).\n", g_pid, signalCode, "");
          }
        }
      }

    #endif

      // 
      // Get a list of child processes for the target pid; these are usually threads.
      // 

      std::vector <pid_t> processThreads;

      if (!GetProcessesThreads (g_pid, processThreads))
      {
        fprintf (stderr, "Failed retrieving threads for PID (%d).\n", g_pid);

        fflush (stderr);

        return ERR_FAILED;
      }

      if (g_verbose)
      {
        printf ("PID (%d) has %zu threads.\n", g_pid, processThreads.size ());
      }
      
      // 
      // Attach to all of the child processes, and synchronise.
      // 

      std::vector <pid_t> attachedThreads;

      std::vector <pid_t> stoppedThreads;

    #if defined (ANDROID)

      for (uint32_t i = 0; i < processThreads.size (); ++i)
      {
        const pid_t thread = processThreads [i];

        if (g_tids.size () > 0)
        {
          // 
          // Skip this thread if it hasn't been selected for profiling.
          // 

          bool skip = true;

          for (size_t t = 0; t < g_tids.size (); ++t)
          {
            if (g_tids [t] == thread)
            {
              skip = false;

              break;
            }
          }
          
          if (skip)
          {
            continue;
          }
        }

        if (g_verbose)
        {
          printf ("Attaching to thread TID (%d)...\n", thread);
        }

        const int attach = PtraceAttach (thread);

        if (attach != -1L)
        {
          int status, result;

          const int options = /*WNOHANG |*/ WUNTRACED | __WALL;

          errno = 0;

          while (true)
          {
            result = waitpid (thread, &status, options);

            if ((result == (pid_t) -1) && (errno == EINTR))
            {
              sched_yield ();

              continue;
            }

            break;
          }

          if (((options & WNOHANG) != 0) && (result == 0))
          {
            continue; // No work.
          }
          else if (((pid_t) result != thread) || (errno != 0))
          {
            fprintf (stderr, "Failed [%d] waiting for thread (%d) attach to settle. %s.\n", result, thread, strerror (errno));

            fflush (stderr);

            continue;
          }
          else if (WIFEXITED (status) || WIFSIGNALED (status))
          {
            if (g_verbose)
            {
              printf ("Discovered thread (%d) has been terminated.\n", thread);
            }

            continue;
          }

          attachedThreads.push_back (thread);

          if (WIFSTOPPED (status))
          {
            const int signalCode = WSTOPSIG (status);

            if (g_verbose)
            {
              printf ("Thread (%u) received signal %d (%s).\n", thread, signalCode, "");
            }

            if (signalCode == SIGSTOP)
            {
              stoppedThreads.push_back (thread);
            }
          }
        }
      }

    #endif

      // 
      // Now processes are attached, and suspended - sample their callstacks.
      // 

      const size_t ignoreDepth = 0, maxDepth = 32;

      static bool requiresLeadingComma = false;

      fprintf (g_frameDataFile, "%s{\"time\":%lld,\"threads\":[", ((requiresLeadingComma) ? "," : ""), GetCurrentTimeNanos ());

      requiresLeadingComma = true;

      for (uint32_t i = 0; i < stoppedThreads.size (); ++i)
      {
        const pid_t thread = stoppedThreads [i];

        if (g_verbose)
        {
          printf ("Starting backtrace sampling for thread (%u)...\n", thread);
        }

        std::string framesJson;

        StackCorkscrew corkscrew;

        const int64_t timeBeganSampling = GetCurrentTimeNanos ();

        const size_t stackDepth = corkscrew.Unwind (g_pid, thread, ignoreDepth, maxDepth);

        lastUpdateTimestampNanos = GetCurrentTimeNanos ();

        if (corkscrew.ConvertToJson (framesJson))
        {
          fprintf (g_frameDataFile, "%s{\"tid\":%d,%s}", ((i > 0) ? "," : ""), thread, framesJson.c_str ());
        }

        if (stackDepth && g_verbose)
        {
          const int64_t samplingDuration = lastUpdateTimestampNanos - timeBeganSampling;

          printf ("Completed thread sampling (%u) in %lld.%lld ns.\n", thread, (samplingDuration / 1000000000LL), (samplingDuration % 1000000000LL));
        }
      }

      fprintf (g_frameDataFile, "]}");

      fflush (g_frameDataFile);

      // 
      // Detach (and continue) an attached process.
      // 

    #if defined (ANDROID)

      for (uint32_t i = 0; i < attachedThreads.size (); ++i)
      {
        const pid_t thread = attachedThreads [i];

        if (g_verbose)
        {
          printf ("Detaching from thread (%u)...\n", thread);
        }

        const long detach = PtraceDetach (thread);
      }

    #endif

      // 
      // We are done with tracing for now; continue the target.
      // 

    #if defined (ANDROID) && 0

      if (suspended)
      {
        if (kill (g_pid, SIGCONT) == -1)
        {
          fprintf (stderr, "Failed signaling SIGCONT to PID (%u). %s.\n", g_pid, strerror (errno));

          fflushf (stderr);

          return ERR_FAILED;
        }

        int status, result;

        do 
        {
          errno = 0;

          while (true)
          {
            result = waitpid (g_pid, &status, WUNTRACED | WCONTINUED);

            if ((result == (pid_t) -1) && (errno == EINTR))
            {
              sched_yield ();

              continue;
            }

            break;
          }

          if ((result != 0) || (errno != 0))
          {
            fprintf (stderr, "Failed waiting on PID (%u). %s.\n", g_pid, strerror (errno));

            fflush (stderr);

            return ERR_FAILED;
          }
          else if (WIFEXITED (status) || WIFSIGNALED (status))
          {
            fprintf (stderr, "Target PID (%d) has terminated; aborting.\n", g_pid);

            fflush (stderr);

            return ERR_FAILED;
          }
        }
        while (WIFSTOPPED (status));
      }

    #endif

    }

    // 
    // Calculate time till next required re-sampling; don't sleep if it's too little time.
    // 

  #ifdef ANDROID

    const int64_t requiredSleepDurationNanos = (lastUpdateTimestampNanos + samplingInterval) - GetCurrentTimeNanos ();

    if (requiredSleepDurationNanos > 3)
    {
      timespec sleepDuration, sleepRemainder;

      sleepRemainder.tv_sec = requiredSleepDurationNanos / 1000000000LL;

      sleepRemainder.tv_nsec = requiredSleepDurationNanos % 1000000000LL;

      do 
      {
        sleepDuration = sleepRemainder;

        int result = nanosleep (&sleepDuration, &sleepRemainder);

        if (result == 0)
        {
          break; // Successfully slept for the specified duration.
        }
        else
        {
          int error = errno;

          fprintf (stderr, "nanosleep failed. %s.\n", strerror (error));

          fflush (stderr);

          errno = 0;

          if (error != EINTR)
          {
            g_terminate = true;

            break;
          }
        }
      }
      while ((sleepRemainder.tv_sec > 0) ||(sleepRemainder.tv_nsec > 0));
    }

  #endif

  }

  fprintf (g_frameDataFile, "]}");

  fflush (g_frameDataFile);

  fclose (g_frameDataFile);

  if (g_verbose)
  {
    printf ("Exiting...");
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

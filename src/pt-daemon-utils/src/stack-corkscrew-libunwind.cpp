////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stack-corkscrew.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define UNW_REMOTE 1

#if defined(__APPLE__)

#include <libunwind.h>

#define UNWIND_SUPPORTED 1

#elif defined (ANDROID) || defined (__linux__)

#include <libunwind.h>

#include <libunwind-ptrace.h>

#define UNWIND_SUPPORTED 1

#else

#define UNWIND_SUPPORTED 0

#endif

#define UNWIND_REMOTE_SUPPORTED (UNWIND_SUPPORTED && !defined(__APPLE__))

#define UNWIND_MAP_SUPPORTED (UNWIND_SUPPORTED && UNWIND_REMOTE_SUPPORTED && 1)

#define UNWIND_STACK_POINTER (UNWIND_SUPPORTED && 1)

#define UNWIND_FUNCTION_NAME (UNWIND_SUPPORTED && 0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t StackCorkscrewLibunwind::Unwind (pid_t ppid, pid_t tid, size_t ignoreDepth, size_t maxDepth)
{
  assert (ppid > 0);

  assert (tid >= ppid);

  m_frames.clear ();

  if (maxDepth == 0)
  {
    return 0;
  }

  static unw_addr_space_t addr_space;

  (void) addr_space;

#if UNWIND_SUPPORTED && UNWIND_REMOTE_SUPPORTED

  addr_space = unw_create_addr_space (&_UPT_accessors, 0);

  if (!addr_space)
  {
    fprintf (stderr, "unw_create_addr_space failed.\n");

    fflush (stderr);

    return 0;
  }

#endif

#if UNWIND_MAP_SUPPORTED && UNWIND_FUNCTION_NAME

  unw_map_cursor_t map_cursor;

  if (unw_map_cursor_create (&map_cursor, tid) < 0)
  {
    fprintf (stderr, "Failed to create map cursor.\n");

    fflush (stderr);

    return 0;
  }

  unw_map_set (addr_space, &map_cursor);

#endif

#if UNWIND_REMOTE_SUPPORTED

  struct UPT_info* upt_info = reinterpret_cast<struct UPT_info*> (_UPT_create (tid));

  if (!upt_info)
  {
    fprintf (stderr, "Failed to create upt info.\n");

    fflush (stderr);

    return 0;
  }

  unw_cursor_t cursor;

  {
    int error = unw_init_remote (&cursor, addr_space, upt_info);

    if (error < 0)
    {
      fprintf (stderr, "unw_init_remote failed (%d)\n", error);

      fflush (stderr);

      return 0;
    }
  }

#endif

  bool shouldContinue = false;

  size_t numFrames = 0;

  do
  {
    //
    // Evaluate instruction pointer / program counter address.
    //

    uint64_t pc = 0;

#if UNWIND_REMOTE_SUPPORTED

    {
      unw_word_t unwound_pc;

      int error = unw_get_reg (&cursor, UNW_REG_IP, &unwound_pc);

      if (error < 0)
      {
        fprintf (stderr, "Failed to read IP (%d)\n", error);

        fflush (stderr);

        break;
      }

      pc = unwound_pc;
    }

#endif

    uint64_t sp = 0;

#if UNWIND_REMOTE_SUPPORTED && UNWIND_STACK_POINTER

    {
      unw_word_t unwound_sp;

      int error = unw_get_reg (&cursor, UNW_REG_SP, &unwound_sp);

      if (error < 0)
      {
        fprintf (stderr, "Failed to read SP (%d)\n", error);

        fflush (stderr);

        break;
      }

      sp = unwound_sp;
    }

#endif

    if (ignoreDepth == 0)
    {
      const char *function = "??";

#if UNWIND_FUNCTION_NAME

      uintptr_t offset = 0;

      char buffer [128];

      unw_word_t value;

      const int result = unw_get_proc_name_by_ip (addr_space, pc, buffer, sizeof (buffer), &value, upt_info);

      if (result >= 0 && buffer [0] != '\0')
      {
        function = buffer;

        offset = static_cast<uintptr_t>(value);
      }

#endif

      StackFrame frame;

      frame.m_level = numFrames;

      frame.m_pc = pc;

#if UNWIND_STACK_POINTER

      frame.m_sp = sp;

#endif

      strncpy (frame.m_function, function, sizeof (frame.m_function));

      m_frames.push_back (frame);

      numFrames++;
    }
    else
    {
      ignoreDepth--;
    }

#if UNWIND_REMOTE_SUPPORTED

    shouldContinue = (unw_step (&cursor) > 0);

#endif
  }
  while (shouldContinue && numFrames < maxDepth);

#if UNWIND_REMOTE_SUPPORTED

  _UPT_destroy (upt_info);

#endif

#if UNWIND_MAP_SUPPORTED && UNWIND_FUNCTION_NAME

  unw_map_cursor_destroy (&map_cursor);

  unw_map_cursor_clear (&map_cursor);

#endif

  return m_frames.size ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

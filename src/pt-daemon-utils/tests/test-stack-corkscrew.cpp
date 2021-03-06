////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include <stack-corkscrew.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (StackCorkscrew, ValidateDefaultStackFrame)
{
  StackFrame stackFrame;

  (void) stackFrame;

  ASSERT_EQ (stackFrame.m_pc, uint64_t (0));

  ASSERT_EQ (stackFrame.m_sp, uint64_t (0));

  ASSERT_EQ (stackFrame.m_level, uint64_t (0));

  ASSERT_TRUE (stackFrame.m_function [0] == '\0');
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (StackCorkscrew, UninitialisedGetFrame)
{
  StackFrame stackFrame;

  StackCorkscrewLibunwind corkscrew;

  ASSERT_EQ (corkscrew.GetDepth (), size_t (0));

  ASSERT_FALSE (corkscrew.GetFrame (-1, &stackFrame));

  ASSERT_FALSE (corkscrew.GetFrame (0, &stackFrame));

  ASSERT_FALSE (corkscrew.GetFrame (256, &stackFrame));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" void NestedUnwindRecurseToDepth (StackCorkscrew &corkscrew, size_t currentDepth, size_t ignoreDepth, size_t nestedDepth)
{
  if (currentDepth < nestedDepth)
  {
    NestedUnwindRecurseToDepth(corkscrew, currentDepth + 1, ignoreDepth, nestedDepth);

    return;
  }

  StackFrame stackFrame;

  const pid_t pid = getpid ();

  const size_t googleTestingFunctionTolerance = 16; // Expecting gtest has no more than 16 functions.

  const size_t depth = corkscrew.Unwind (pid, pid, ignoreDepth, nestedDepth + googleTestingFunctionTolerance);

  if (depth > 0)
  {
    ASSERT_TRUE (corkscrew.GetFrame (0, &stackFrame)); // Unwind ()

    ASSERT_TRUE (corkscrew.GetFrame (1, &stackFrame));

    ASSERT_TRUE (strcmp (stackFrame.m_function, "NestedUnwindRecurseToDepth") == 0);

    ASSERT_TRUE (corkscrew.GetFrame (depth - 1, &stackFrame));

    ASSERT_TRUE (strcmp (stackFrame.m_function, "main") == 0);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void UnwindUsingCommonTests (StackCorkscrew &corkscrew)
{
  const size_t ignoreDepth = 0;

  const size_t maxDepth = 12;

  NestedUnwindRecurseToDepth (corkscrew, 0, ignoreDepth, maxDepth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (StackCorkscrew, UnwindUsingLibunwind)
{
  StackCorkscrewLibunwind corkscrew;

  UnwindUsingCommonTests (corkscrew);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (StackCorkscrew, UnwindUsingLibcppabi)
{
  StackCorkscrewLibcppabi corkscrew;

  UnwindUsingCommonTests (corkscrew);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv [])
{
  testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

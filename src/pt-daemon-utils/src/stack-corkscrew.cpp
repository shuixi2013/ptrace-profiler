////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stack-corkscrew.h"

#include <algorithm>

#include <cassert>

#include <cstring>

#include <errno.h>

#include <stdlib.h>

#include <stdio.h>

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackCorkscrew::StackCorkscrew ()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackCorkscrew::~StackCorkscrew ()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool StackCorkscrew::PopulateJsonObject (JsonNode &node) const
{
  if (!node.IsObject ())
  {
    assert (node.IsObject ());

    return false;
  }

  kvr::value *array = node.GetImpl ()->insert_array ("frames");

  JsonNodeKvr arrayNode (*array);

  return PopulateJsonArray (arrayNode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool StackCorkscrew::PopulateJsonArray (JsonNode &node) const
{
  if (!node.IsArray ())
  {
    assert (node.IsArray ());

    return false;
  }

  for (size_t i = 0; i < m_frames.size (); ++i)
  {
    const StackFrame &frame = m_frames [i];

    kvr::value *frameObj = node.GetImpl ()->push_map ();

    if (!frameObj)
    {
      assert (frameObj);

      continue;
    }

    //
    // We need to explicitly convert values to strings as we don't want to risk differences
    // in arithmetic conversions between intptr_t and uintptr_t register addresses.
    //

    char buffer [32];

    snprintf (buffer, 32, "%" PRIu64, frame.m_level);

    frameObj->insert ("level", buffer);

    snprintf (buffer, 32, "%" PRIu64, frame.m_pc);

    frameObj->insert ("pc", buffer);

    snprintf (buffer, 32, "%" PRIu64, frame.m_sp);

    frameObj->insert ("sp", buffer);

    frameObj->insert ("func", frame.m_function);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool StackCorkscrew::GetFrame (size_t index, StackFrame *frame) const
{
  if (!frame)
  {
    return false;
  }

  if (index >= m_frames.size ())
  {
    return false;
  }

  *frame = m_frames [index];

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool StackCorkscrew::PushFrame (const StackFrame &frame)
{
  m_frames.push_back (frame);

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t StackCorkscrew::GetDepth () const
{
  return m_frames.size ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

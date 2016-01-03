////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "json_utils.h"

#include <cassert>

#include <cstring>

#include <errno.h>

#include <stdlib.h>

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonNode::~JsonNode ()
{
  if (m_node && m_shouldDeref)
  {
    json_decref (m_node);

    m_node = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetString (char *buffer, size_t bufferSize) const
{
  if (IsString ())
  {
    const char *string = json_string_value (m_node);

    strncpy (buffer, string, bufferSize - 1);

    buffer [bufferSize - 1] = '\0';

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetString (std::string &output) const
{
  if (IsString ())
  {
    output = json_string_value (m_node);

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetInteger (int32_t &output) const
{
  if (IsInteger ())
  {
    output = (int32_t) json_integer_value (m_node);

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetInteger (int64_t &output) const
{
  if (IsInteger ())
  {
    output = (int64_t) json_integer_value (m_node);

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetDouble (double &output) const
{
  if (IsDouble ())
  {
    output = json_real_value (m_node);

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::GetBool (bool &output) const
{
  if (IsDouble ())
  {
    output = IsTrue ();

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t JsonNode::GetLength () const
{
  assert (IsValid ());

  if (IsArray ())
  {
    return json_array_size (m_node);
  }
  else if (IsObject ())
  {
    return json_object_size (m_node);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::TryChild (const char *key, JsonNode *child) const
{
  const char *path [] = { key, NULL };

  return TryChild (path, child);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::TryChild (const char *path [], JsonNode *child) const
{
  return TryChild (path, json_typeof (m_node), child);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::TryChild (const char *path [], json_type type, JsonNode *child) const
{
  if (m_node == NULL)
  {
    return false;
  }

  json_t *node = m_node;

  if (type == JSON_OBJECT)
  {
    size_t i = 0;

    while (path [i] != NULL)
    {
      const char *key = path [i++];

      node = json_object_get (node, key); // borrowed value
    }
  }

  if (node != NULL)
  {
    *child = JsonNode (node);

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonNode::TryChildNodeAtIndex (size_t index, JsonNode *child) const
{
  if (IsValid () && index < GetLength ())
  {
    if (IsArray ())
    {
      json_t *node = json_array_get (m_node, index);

      JsonNode arrayNode (node, false); // borrowed value

      *child = arrayNode;

      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonNode *JsonUtils::LoadFromFile (const char *filename)
{
  // 
  // Parse JSON data by loading in the specified file in its entirety.
  // 

  FILE *jsonFile = fopen (filename, "r");

  if (!jsonFile)
  {
    fprintf (stderr, "Failed to open profile data file (%s). %s.\n", filename, strerror (errno));

    fflush (stderr);

    errno = 0;

    return NULL;
  }

  json_error_t error;

  json_t *node = json_loadf (jsonFile, JSON_DECODE_ANY, &error);

  if (!node)
  {
    fprintf (stderr, "Failed reading entire profile data file. %s (%s@%d,%d).\n", error.text, error.source, error.line, error.column);

    fflush (stderr);

    errno = 0;

    return NULL;
  }

  fclose (jsonFile);

  return new JsonNode (node, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonNode *JsonUtils::LoadFromMemory (const char *data, size_t dataSize)
{
  // 
  // Parse JSON data provided directly from a memory block.
  // 

  assert (data != NULL);

  assert (dataSize > 0);

  if (data && dataSize)
  {
    json_error_t error;

    json_t *node = json_loadb (data, dataSize, JSON_DECODE_ANY, &error);

    if (node == NULL)
    {
      fprintf (stderr, "Failed parsing buffered JSON data. %s (pos: %d).\n", error.text, error.position);

      fflush (stderr);

      return NULL;
    }

    return new JsonNode (node, true);
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsString (const JsonNode &node, const char *key, char *buffer, size_t bufferSize)
{
  const char *path [] = { key, NULL };

  return GetChildAsString (node, path, buffer, bufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsString (const JsonNode &node, const char *key, std::string &output)
{
  const char *path [] = { key, NULL };

  return GetChildAsString (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsInteger (const JsonNode &node, const char *key, int32_t &output)
{
  const char *path [] = { key, NULL };

  return GetChildAsInteger (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsInteger (const JsonNode &node, const char *key, int64_t &output)
{
  const char *path [] = { key, NULL };

  return GetChildAsInteger (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsDouble (const JsonNode &node, const char *key, double &output)
{
  const char *path [] = { key, NULL };

  return GetChildAsDouble (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsBool (const JsonNode &node, const char *key, bool &output)
{
  const char *path [] = { key, NULL };

  return GetChildAsBool (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsString (const JsonNode &node, const char *path [], char *buffer, size_t bufferSize)
{
  const bool success = TryChildAsString (node, path, buffer, bufferSize);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsString (const JsonNode &node, const char *path [], std::string &output)
{
  const bool success = TryChildAsString (node, path, output);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsInteger (const JsonNode &node, const char *path [], int32_t &output)
{
  const bool success = TryChildAsInteger (node, path, output);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsInteger (const JsonNode &node, const char *path [], int64_t &output)
{
  const bool success = TryChildAsInteger (node, path, output);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsDouble (const JsonNode &node, const char *path [], double &output)
{
  const bool success = TryChildAsDouble (node, path, output);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::GetChildAsBool (const JsonNode &node, const char *path [], bool &output)
{
  const bool success = TryChildAsBool (node, path, output);

  assert (success);

  return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsString (const JsonNode &node, const char *key, char *buffer, size_t bufferSize)
{
  const char *path [] = { key, NULL };

  return TryChildAsString (node, path, buffer, bufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsString (const JsonNode &node, const char *key, std::string &output)
{
  const char *path [] = { key, NULL };

  return TryChildAsString (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsInteger (const JsonNode &node, const char *key, int32_t &output)
{
  const char *path [] = { key, NULL };

  return TryChildAsInteger (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsInteger (const JsonNode &node, const char *key, int64_t &output)
{
  const char *path [] = { key, NULL };

  return TryChildAsInteger (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsDouble (const JsonNode &node, const char *key, double &output)
{
  const char *path [] = { key, NULL };

  return TryChildAsDouble (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsBool (const JsonNode &node, const char *key, bool &output)
{
  const char *path [] = { key, NULL };

  return TryChildAsBool (node, path, output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsString (const JsonNode &node, const char *path [], char *buffer, size_t bufferSize)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsString ())
  {
    return false;
  }

  return child.GetString (buffer, bufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsString (const JsonNode &node, const char *path [], std::string &output)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsString ())
  {
    return false;
  }

  return child.GetString (output);;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsInteger (const JsonNode &node, const char *path [], int32_t &output)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsInteger ())
  {
    return false;
  }

  return child.GetInteger (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsInteger (const JsonNode &node, const char *path [], int64_t &output)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsInteger ())
  {
    return false;
  }

  return child.GetInteger (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsDouble (const JsonNode &node, const char *path [], double &output)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsDouble ())
  {
    return false;
  }

  return child.GetDouble (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::TryChildAsBool (const JsonNode &node, const char *path [], bool &output)
{
  JsonNode child;

  const bool success = node.TryChild (path, &child);

  if (!success || !child.IsBool ())
  {
    return false;
  }

  return child.GetBool (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
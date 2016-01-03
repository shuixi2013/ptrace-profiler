////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "json_utils.h"

#include <cassert>

#include <cstring>

#include <stdio.h>

#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv [])
{
  // 
  // Validate parsing of rudimentary JSON object block.
  // 

  {
    const char *emptyObject = "{}";

    JsonNode *emptyObjectNode = JsonUtils::LoadFromMemory (emptyObject, sizeof (char) * strlen (emptyObject)); // non-terminated

    if (!emptyObjectNode)
    {
      fprintf (stderr, "Failed loading of empty JSON object block.\n");

      fflush (stderr);

      return 1;
    }

    assert (emptyObjectNode->IsValid ());

    assert (emptyObjectNode->IsObject ());

    assert (emptyObjectNode->GetLength () == 0);

    delete emptyObjectNode;
  }

  // 
  // Validate parsing of rudimentary JSON array block.
  // 

  {
    const char *emptyArray = "[]";

    JsonNode *emptyArrayNode = JsonUtils::LoadFromMemory (emptyArray, sizeof (char) * strlen (emptyArray)); // non-terminated

    if (!emptyArrayNode)
    {
      fprintf (stderr, "Failed loading of empty JSON array block.\n");

      fflush (stderr);

      return 1;
    }

    assert (emptyArrayNode->IsValid ());

    assert (emptyArrayNode->IsArray ());

    assert (emptyArrayNode->GetLength () == 0);

    delete emptyArrayNode;
  }

  // 
  // Validate parsing of a mixture objects and array data.
  // 

  {
    const char *complexObject = "{\"test1\":[\"data1\",\"data2\"],\"test2\":\"string1\"}";

    JsonNode *complexObjectNode = JsonUtils::LoadFromMemory (complexObject, sizeof (char) * strlen (complexObject)); // non-terminated

    if (!complexObjectNode)
    {
      fprintf (stderr, "Failed loading of empty JSON array block.\n");

      fflush (stderr);

      return 1;
    }

    assert (complexObjectNode->IsValid ());

    assert (complexObjectNode->IsObject ());

    assert (complexObjectNode->GetLength () == 2);

    JsonNode test1Node, data1Node;

    if (!complexObjectNode->TryChild ("test1", &test1Node))
    {
      fprintf (stderr, "Failed retrieval of 'test1' node.\n");

      fflush (stderr);

      return 1;
    }

    if (!test1Node.TryChildNodeAtIndex (0, &data1Node))
    {
      fprintf (stderr, "Failed retrieval of 'data1' child node.\n");

      fflush (stderr);

      return 1;
    }

    std::string data1NodeString;

    if (!data1Node.GetString (data1NodeString))
    {
      fprintf (stderr, "Failed retrieval of 'data1' child data.\n");

      fflush (stderr);

      return 1;
    }

    const char *expectedStringValue = "data1";

    if (data1NodeString.compare (expectedStringValue) != 0)
    {
      fprintf (stderr, "Retrieved 'data1' child data (%s) doesn't match expected value (%s).\n", data1NodeString.c_str (), expectedStringValue);

      fflush (stderr);

      return 1;
    }

    delete complexObjectNode;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

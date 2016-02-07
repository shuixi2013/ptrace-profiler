#include <json-node.h>

JsonNodeKvr::JsonNodeKvr (kvr::ctx *context)
  : JsonNodeInterface <kvr::value> (*context->create_value ())
{
}

JsonNodeKvr::JsonNodeKvr (kvr::value *node)
  : JsonNodeInterface <kvr::value> (node)
{

}

JsonNodeKvr::JsonNodeKvr (const kvr::value *node)
  : JsonNodeInterface <kvr::value> (node)
{

}

JsonNodeKvr::JsonNodeKvr (const kvr::value &node)
  : JsonNodeInterface <kvr::value> (node)
{

}

JsonNodeKvr::~JsonNodeKvr ()
{

}
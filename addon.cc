// addon.cc
#include <node.h>
#include "node-wrapper.h"

namespace gifencoder
{

using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports, Local<Object> module)
{
  NodeWrapper::Init(exports, module);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

} // namespace gifencoder
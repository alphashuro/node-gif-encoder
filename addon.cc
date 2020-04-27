// addon.cc
#include <node.h>
#include "gif-encoder.h"

namespace gifencoder {

using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports, Local<Object> module) {
  GIFEncoder::Init(exports, module);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}  // namespace demo
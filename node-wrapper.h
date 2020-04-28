#ifndef NODEWRAPPER_H
#define NODEWRAPPER_H

#include <node.h>
#include <node_object_wrap.h>
#include "gif-encoder.h"

namespace gifencoder
{
class NodeWrapper : public node::ObjectWrap
{
private:
  GIFEncoder encoder;

public:
  NodeWrapper(int width, int height)
  {
    encoder = GIFEncoder(width, height);
  }

  static void Init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module);

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Start(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Finish(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void SetRepeat(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void SetQuality(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void SetFrameRate(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void AddFrame(const v8::FunctionCallbackInfo<v8::Value> &args);
};
} // namespace gifencoder

#endif
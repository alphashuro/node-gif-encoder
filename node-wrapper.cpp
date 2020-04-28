#include "node-wrapper.h"
#include "node_buffer.h"

namespace gifencoder
{
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::String;
using v8::Value;

void NodeWrapper::Init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module)
{
  Isolate *isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);
  Local<Object> addon_data = addon_data_tpl->NewInstance(context).ToLocalChecked();

  // prep constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New, addon_data);
  tpl->SetClassName(String::NewFromUtf8(isolate, "GIFEncoder", NewStringType::kNormal).ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setRepeat", SetRepeat);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setQuality", SetQuality);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setFrameRate", SetFrameRate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addFrame", AddFrame);
  NODE_SET_PROTOTYPE_METHOD(tpl, "finish", Finish);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  // addon_data->SetInternalField(0, constructor);
  module->Set(context, String::NewFromUtf8(isolate, "exports", NewStringType::kNormal).ToLocalChecked(), constructor).FromJust();
};
void NodeWrapper::New(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall())
  {
    // invoked using `new`
    int width = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);
    int height = args[1]->IsUndefined() ? 0 : args[1]->NumberValue(context).FromMaybe(0);

    NodeWrapper *wrapper = new NodeWrapper(width, height);

    wrapper->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }
  else
  {
    // invoked as plain function `GIFEncoder()`
    const int argc = 2;
    Local<Value> argv[argc] = {
        args[0],
        args[1]};
    Local<Function> cons = args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> result = cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
};
void NodeWrapper::Start(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  wrapper->encoder.start();
};

void NodeWrapper::SetRepeat(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  int repeat = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);

  wrapper->encoder.setRepeat(repeat);
};
void NodeWrapper::SetQuality(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  int quality = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);

  wrapper->encoder.setQuality(quality);
};

void NodeWrapper::SetFrameRate(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  int fps = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);

  wrapper->encoder.setFrameRate(fps);
};

void NodeWrapper::AddFrame(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  char *imageData = node::Buffer::Data(args[0]);
  size_t length = node::Buffer::Length(args[0]);

  vector<char> img(imageData, imageData + length);

  wrapper->encoder.addFrame(img);
};

void NodeWrapper::Finish(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  NodeWrapper *wrapper = ObjectWrap::Unwrap<NodeWrapper>(args.Holder());

  wrapper->encoder.finish();

  char *d = reinterpret_cast<char *>(wrapper->encoder.out.data.data());

  Local<Object>
      buf;

  node::Buffer::Copy(
      isolate,
      d,
      wrapper->encoder.out.data.size())
      .ToLocal(&buf);

  args.GetReturnValue()
      .Set(buf);
}

} // namespace gifencoder
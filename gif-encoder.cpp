#include "gif-encoder.h"

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

GIFEncoder::GIFEncoder(int w, int h) : width(w), height(h){};

GIFEncoder::~GIFEncoder(){};

void GIFEncoder::Init(Local<Object> exports, Local<Object> module)
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

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  // addon_data->SetInternalField(0, constructor);
  module->Set(context, String::NewFromUtf8(isolate, "exports", NewStringType::kNormal).ToLocalChecked(), constructor).FromJust();
}

void GIFEncoder::New(const FunctionCallbackInfo<Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall())
  {
    // invoked using `new`
    int width = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);
    int height = args[1]->IsUndefined() ? 0 : args[1]->NumberValue(context).FromMaybe(0);

    GIFEncoder *encoder = new GIFEncoder(width, height);
    encoder->Wrap(args.This());
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
}

void GIFEncoder::Start(const FunctionCallbackInfo<Value> &args)
{
  GIFEncoder *encoder = ObjectWrap::Unwrap<GIFEncoder>(args.Holder());

  string s = "GIF89a";

  transform(
      s.begin(),
      s.end(),
      encoder->out.data.begin(),
      [](char c) { return byte(c); });

  encoder->started = true;
}

void GIFEncoder::SetRepeat(const FunctionCallbackInfo<Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  GIFEncoder *encoder = ObjectWrap::Unwrap<GIFEncoder>(args.Holder());

  encoder->repeat = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);
}

void GIFEncoder::SetQuality(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  GIFEncoder *encoder = ObjectWrap::Unwrap<GIFEncoder>(args.Holder());

  int quality = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);
  if (quality < 1)
    quality = 1;

  encoder->sample = quality;
}

void GIFEncoder::SetFrameRate(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  GIFEncoder *encoder = ObjectWrap::Unwrap<GIFEncoder>(args.Holder());

  double fps = args[0]->IsUndefined() ? 0 : args[0]->NumberValue(context).FromMaybe(0);

  encoder->sample = int(round(100 / fps));
}

void GIFEncoder::AddFrame(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  GIFEncoder *encoder = ObjectWrap::Unwrap<GIFEncoder>(args.Holder());

  char *imageData = node::Buffer::Data(args[0]);
  size_t length = node::Buffer::Length(args[0]);

  vector<char> img(imageData, imageData + length);

  encoder->image.resize(length);

  transform(img.begin(), img.end(), encoder->image.begin(), [](char c) {
    return byte(c);
  });

  encoder->getImagePixels(); // convert to correct format if necessary
  encoder->analyzePixels();  // build color table & map pixels

  if (encoder->firstFrame)
  {
    encoder->writeLSD();     // logical screen descriptior
    encoder->writePalette(); // global color table
    if (encoder->repeat >= 0)
    {
      // use NS app extension to indicate reps
      encoder->writeNetscapeExt();
    }
  }

  encoder->writeGraphicCtrlExt(); // write graphic control extension
  encoder->writeImageDesc();      // image descriptor
  if (!encoder->firstFrame)
    encoder->writePalette(); // local color table
  encoder->writePixels();    // encode and write pixel data

  encoder->firstFrame = false;
}

void GIFEncoder::getImagePixels()
{
  int size = width * height * 3;
  byte p[size];

  int count = 0;

  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      int b = (i * width * 4) + j * 4;

      p[count++] = byte(image[b]);
      p[count++] = image[b + 1];
      p[count++] = image[b + 2];
    }
  }

  pixels.assign(p, p + size);
}

void GIFEncoder::writePixels()
{
  LZWEncoder enc = LZWEncoder(width, height, indexedPixels, colorDepth);
  enc.encode(out);
}

void GIFEncoder::analyzePixels()
{
  int len = pixels.size();
  int nPix = len / 3;

  byte indexedPixels[nPix];

  TypedNeuQuant imgq(pixels, sample);
  imgq.buildColormap(); // create reduced palette
  colorTab = imgq.getColormap();

  // map image pixels to new palette
  int k = 0;
  for (int j = 0; j < nPix; j++)
  {
    int index = imgq.lookupRGB(
        pixels[k++] & 0xff,
        pixels[k++] & 0xff,
        pixels[k++] & 0xff);
    usedEntry[index] = true;
    indexedPixels[j] = byte(index);
  }

  pixels.clear();
  colorDepth = 8;
  palSize = 7;

  // get closest match to transparent color if specified
  if (transparent.has_value())
  {
    transIndex = findClosest(transparent.value());

    // ensure that pixels with full transparency in the RGBA image are using the selected transparent color index in the indexed image.
    for (int pixelIndex = 0; pixelIndex < nPix; pixelIndex++)
    {
      if (image[pixelIndex * 4 + 3] == byte(0))
      {
        indexedPixels[pixelIndex] = byte(transIndex);
      }
    }
  }
}

/*
  Returns index of palette color closest to c
*/
int GIFEncoder::findClosest(byte c)
{
  if (colorTab.size() == 0)
    return -1;

  byte r = (c & 0xFF0000) >> 16;
  byte g = (c & 0x00FF00) >> 8;
  byte b = (c & 0x0000FF);
  int minpos = 0;
  byte dmin = byte(256 * 256 * 256);
  int len = colorTab.size();

  for (int i = 0; i < len;)
  {
    int index = i / 3;
    byte dr = r - (colorTab[i++] & 0xff);
    byte dg = g - (colorTab[i++] & 0xff);
    byte db = b - (colorTab[i++] & 0xff);
    byte d = dr * dr + dg * dg + db * db;
    if (usedEntry[index] && (d < dmin))
    {
      dmin = d;
      minpos = index;
    }
  }

  return minpos;
};

/*
  Writes Logical Screen Descriptor
*/
void GIFEncoder::writeLSD()
{
  // logical screen size
  writeShort(byte(width));
  writeShort(byte(height));

  // packed fields
  out.writeByte(
      0x80 |  // 1 : global color table flag = 1 (gct used)
      0x70 |  // 2-4 : color resolution = 7
      0x00 |  // 5 : gct sort flag = 0
      palSize // 6-8 : gct size
  );

  out.writeByte(0); // background color index
  out.writeByte(0); // pixel aspect ratio - assume 1:1
};

void GIFEncoder::writeShort(byte pValue)
{
  out.writeByte(pValue & 0xFF);
  int np = int(pValue) >> 8;
  int nb = np & 0xFF;
  out.writeByte(nb);
};

void GIFEncoder::writeShort(int pValue)
{
  writeShort(byte(pValue));
};

void GIFEncoder::writePalette()
{
  out.writeBytes(colorTab);
  int n = (3 * 256) - colorTab.size();
  for (int i = 0; i < n; i++)
    out.writeByte(0);
}

/*
  Writes Netscape application extension to define repeat count.
*/
void GIFEncoder::writeNetscapeExt()
{
  out.writeByte(0x21);              // extension introducer
  out.writeByte(0xff);              // app extension label
  out.writeByte(11);                // block size
  out.writeUTFBytes("NETSCAPE2.0"); // app id + auth code
  out.writeByte(3);                 // sub-block size
  out.writeByte(1);                 // loop sub-block id
  writeShort(byte(repeat));         // loop count (extra iterations, 0=repeat forever)
  out.writeByte(0);                 // block terminator
};

/*
  Writes Graphic Control Extension
*/
void GIFEncoder::writeGraphicCtrlExt()
{
  out.writeByte(0x21); // extension introducer
  out.writeByte(0xf9); // GCE label
  out.writeByte(4);    // data block size

  int transp, disp;
  if (!transparent.has_value())
  {
    transp = 0;
    disp = 0; // dispose = no action
  }
  else
  {
    transp = 1;
    disp = 2; // force clear if using transparent color
  }

  if (dispose >= 0)
  {
    disp = dispose & 7; // user override
  }
  disp <<= 2;

  // packed fields
  out.writeByte(
      0 |    // 1:3 reserved
      disp | // 4:6 disposal
      0 |    // 7 user input - 0 = none
      transp // 8 transparency flag
  );

  writeShort(byte(delay));   // delay x 1/100 sec
  out.writeByte(transIndex); // transparent color index
  out.writeByte(0);          // block terminator
};

/*
  Writes Image Descriptor
*/
void GIFEncoder::writeImageDesc()
{
  out.writeByte(0x2c); // image separator
  writeShort(0);       // image position x,y = 0,0
  writeShort(0);
  writeShort(width); // image size
  writeShort(height);

  // packed fields
  if (firstFrame)
  {
    // no LCT - GCT is used for first (or only) frame
    out.writeByte(0);
  }
  else
  {
    // specify normal LCT
    out.writeByte(
        0x80 |  // 1 local color table 1=yes
        0 |     // 2 interlace - 0=no
        0 |     // 3 sorted - 0=no
        0 |     // 4-5 reserved
        palSize // 6-8 size of color table
    );
  }
};
} // namespace gifencoder
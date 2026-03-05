#include <node.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace demo {
using v8::FunctionCallbackInfo; using v8::Isolate; using v8::Local;
using v8::Object; using v8::String; using v8::Value;

void ReadHex(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  String::Utf8Value path(isolate, args[0]);
  std::string filePath(*path);
  
  std::ifstream file(filePath, std::ios::binary);
  if(!file.is_open()) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: Access Denied").ToLocalChecked());
    return;
  }

  std::vector<unsigned char> buffer(512);
  file.read((char*)buffer.data(), buffer.size());
  std::streamsize bytesRead = file.gcount();

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < bytesRead; ++i)
    ss << std::setw(2) << (int)buffer[i];

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, ss.str().c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "readHex", ReadHex);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, init)
}

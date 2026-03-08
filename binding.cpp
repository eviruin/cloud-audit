#include <napi.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <cstdio>
#include <memory>

using namespace Napi;

std::string raw_exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "Error popen";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) result += buffer.data();
    return result;
}

Value ListDir(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    DIR *dir;
    struct dirent *ent;
    std::string result = "";
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            result += std::string(ent->d_name) + "\n";
        }
        closedir(dir);
    } else { result = "DENIED: " + path; }
    return String::New(env, result);
}

Value ReadFile(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    std::ifstream file(path);
    if (!file.is_open()) return String::New(env, "READ_DENIED");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return String::New(env, buffer.str());
}

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    std::string command = info[0].As<String>().Utf8Value();
    return String::New(env, raw_exec(command.c_str()));
}

Object Init(Env env, Object exports) {
    exports.Set("execute", Function::New(env, Execute));
    exports.Set("listDir", Function::New(env, ListDir));
    exports.Set("readFile", Function::New(env, ReadFile));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

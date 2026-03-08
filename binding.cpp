#include <napi.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

Napi::Value ProbePath(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string path = info[0].As<Napi::String>();

    struct stat st;
    Napi::Object res = Napi::Object::New(env);

    res.Set("exists", Napi::Boolean::New(env, access(path.c_str(), F_OK) == 0));
    res.Set("readable", Napi::Boolean::New(env, access(path.c_str(), R_OK) == 0));

    if (stat(path.c_str(), &st) == 0) {
        res.Set("mode", Napi::Number::New(env, st.st_mode));
        res.Set("uid", Napi::Number::New(env, st.st_uid));
    } else {
        res.Set("error", Napi::String::New(env, strerror(errno)));
    }

    return res;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "probe"), Napi::Function::New(env, ProbePath));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

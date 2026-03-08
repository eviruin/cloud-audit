#include <napi.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <vector>

using namespace Napi;

Value ReadFileRaw(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return String::New(env, "SYSCALL_OPEN_FAILED");

    char buffer[4096];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytesRead < 0) return String::New(env, "SYSCALL_READ_FAILED");
    buffer[bytesRead] = '\0';
    return String::New(env, buffer);
}

Value ListDir(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    DIR *dir = opendir(path.c_str());
    if (!dir) return String::New(env, "OPENDIR_FAILED");

    std::string result = "";
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        result += std::string(entry->d_name) + "\n";
    }
    closedir(dir);
    return String::New(env, result);
}

Value Execute(const CallbackInfo& info) {
    Env env = info.Env();
    std::string cmd = info[0].As<String>().Utf8Value();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return String::New(env, "POPEN_FAILED");
    char buffer[128];
    std::string res = "";
    while (fgets(buffer, 128, pipe)) res += buffer;
    pclose(pipe);
    return String::New(env, res);
}

Object Init(Env env, Object exports) {
    exports.Set("execute", Function::New(env, Execute));
    exports.Set("readFile", Function::New(env, ReadFileRaw));
    exports.Set("listDir", Function::New(env, ListDir));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

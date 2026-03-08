#include <napi.h>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

Napi::String ReadSystemFile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string path = info[0].As<Napi::String>();

    std::ifstream file(path);
    if (!file.is_open()) return Napi::String::New(env, "ERR_ACCESS_DENIED");

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return Napi::String::New(env, content);
}

Napi::String ProbeInternal(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string ip = info[0].As<Napi::String>();
    int port = info[1].As<Napi::Number>().Int32Value();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    struct timeval tv;
    tv.tv_sec = 1; tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        close(sock);
        return Napi::String::New(env, "OFFLINE");
    }
    close(sock);
    return Napi::String::New(env, "ONLINE_OPEN");
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("read", Napi::Function::New(env, ReadSystemFile));
    exports.Set("probe", Napi::Function::New(env, ProbeInternal));
    return exports;
}
NODE_API_MODULE(hardcore_probe, Init)

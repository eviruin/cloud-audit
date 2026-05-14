#include <node.h>
#include <v8.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <vector>

using namespace v8;

void ScanHost(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string result = "[*] Starting Native Deep Scan...\n";

    // 1. Coba brute force FD yang mungkin di-inherit dari Docker Parent
    for(int i = 3; i < 15; i++) {
        char path[256];
        if (fcntl(i, F_GETFL) != -1) {
            sprintf(path, "/proc/self/fd/%d", i);
            char actualpath[1024];
            char* ptr = realpath(path, actualpath);
            if (ptr) {
                result += "[+] Found Inherited FD " + std::to_string(i) + " -> " + std::string(actualpath) + "\n";
            }
        }
    }

    // 2. Coba lakukan 'openat' pada directory yang biasanya dilarang
    int root_fd = open("/", O_RDONLY);
    if (root_fd != -1) {
        int shadow_fd = openat(root_fd, "etc/shadow", O_RDONLY);
        if (shadow_fd != -1) {
            result += "[!!!] CRITICAL: openat(etc/shadow) SUCCESS!\n";
            close(shadow_fd);
        }
        close(root_fd);
    }

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "scanHost", ScanHost);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

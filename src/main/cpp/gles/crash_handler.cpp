// ===========================================================================
// src/main/cpp/loader_stub.cpp → 现在是 crash_handler.cpp
//
// SIGSEGV / SIGBUS / SIGILL handler, 在崩点把 native 栈写到 /data/local/tmp/
// 这样下次崩时不用猜, 看到栈就知道哪行代码错.
// ===========================================================================
#include "vmcr/log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cxxabi.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <ucontext.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unwind.h>

namespace {

constexpr const char* kTag = "VMCR-Signal";

struct BacktraceState {
    int fd = -1;
    int frame = 0;
    bool ok = true;
};

bool file_exists(const char* path) {
    struct stat st;
    return ::stat(path, &st) == 0;
}

bool ensure_dir(const char* path) {
    if (file_exists(path)) return true;
    return ::mkdir(path, 0777) == 0;
}

const char* signal_name(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGBUS:  return "SIGBUS";
        case SIGILL:  return "SIGILL";
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        default:      return "UNKNOWN";
    }
}

_Unwind_Reason_Code unwind_callback(_Unwind_Context* ctx, void* arg) {
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(ctx);
    if (state->frame == 0) {
        ::dprintf(state->fd, "\n  frame #0 (PC=0x%016lx)\n", (unsigned long)pc);
    } else {
        Dl_info info;
        if (::dladdr((void*)pc, &info) && info.dli_fname) {
            const char* sym = info.dli_sname;
            char* demangled = sym ? abi::__cxa_demangle(sym, nullptr, nullptr, nullptr) : nullptr;
            const char* lib = info.dli_fname ? info.dli_fname : "?";
            uintptr_t base = (uintptr_t)info.dli_fbase;
            uintptr_t off = pc - base;
            if (demangled) {
                ::dprintf(state->fd, "  frame #%d: %p  %s+%p  (%s)\n",
                          state->frame, (void*)pc, demangled, (void*)off, lib);
                ::free(demangled);
            } else {
                ::dprintf(state->fd, "  frame #%d: %p  %s+%p  (%s)\n",
                          state->frame, (void*)pc,
                          sym ? sym : "??", (void*)off, lib);
            }
        } else {
            ::dprintf(state->fd, "  frame #%d: %p  (no dladdr)\n", state->frame, (void*)pc);
        }
    }
    state->frame++;
    if (state->frame > 64) return _URC_END_OF_STACK;  // limit
    return _URC_NO_REASON;
}

void write_backtrace(int fd, ucontext_t* ctx) {
    ::dprintf(fd, "--- VMCR backtrace (pid=%d, tid=%d) ---\n", ::getpid(), ::gettid());
    BacktraceState state;
    state.fd = fd;
    _Unwind_Backtrace(unwind_callback, &state);
    ::dprintf(fd, "--- end backtrace ---\n");
}

void signal_handler(int sig, siginfo_t* info, void* ucontext) {
    // 重定向到 stderr + 文件
    const char* paths[] = {
        "/data/local/tmp/vmcr_crash",
        "/sdcard/Android/data/com.mio.plugin.renderer.vmcr/files/vmcr_crash",
    };

    int saved_errno = errno;
    for (const char* p : paths) {
        int fd = ::open(p, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0) {
            ::dprintf(fd, "=== VMCR native crash dump ===\n");
            ::dprintf(fd, "signal: %d (%s)\n", sig, signal_name(sig));
            ::dprintf(fd, "si_code: %d\n", info ? info->si_code : -1);
            ::dprintf(fd, "si_addr: %p\n", info ? info->si_addr : nullptr);
            ::dprintf(fd, "pid: %d  tid: %d\n", ::getpid(), ::gettid());
            write_backtrace(fd, static_cast<ucontext_t*>(ucontext));
            ::dprintf(fd, "=== end ===\n");
            ::close(fd);
            ::chmod(p, 0666);  // adb pull 友好
        }
    }
    // 也写 stderr
    ::fprintf(stderr, "[VMCR-Signal] %s caught! Dumped to /data/local/tmp/vmcr_crash\n", signal_name(sig));
    ::fflush(stderr);
    errno = saved_errno;

    // 重新抛信号, 让 Android tombstone 也生成
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    ::sigemptyset(&sa.sa_mask);
    ::sigaction(sig, &sa, nullptr);
    ::raise(sig);
}

void install_handlers() {
    static const int kSigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGABRT, SIGFPE };
    struct sigaction sa{};
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    ::sigemptyset(&sa.sa_mask);
    for (int sig : kSigs) {
        ::sigaction(sig, &sa, nullptr);
    }
    // 也写标记文件证明 handler 装好了
    const char* marker = "/data/local/tmp/vmcr_signal_handler";
    int fd = ::open(marker, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        const char* msg = "VMCR signal handler installed\n";
        ::write(fd, msg, ::strlen(msg));
        ::close(fd);
        ::chmod(marker, 0666);
    }
    LOG_I(kTag, "Signal handlers installed for SIGSEGV/SIGBUS/SIGILL/SIGABRT/SIGFPE");
}

}  // namespace

// 构造器 + 析构器
extern "C" __attribute__((constructor))
void vmcr_crash_handler_ctor() {
    install_handlers();
}

extern "C" __attribute__((destructor))
void vmcr_crash_handler_dtor() {
    LOG_I(kTag, "VMCR libGL.so unloading");
}

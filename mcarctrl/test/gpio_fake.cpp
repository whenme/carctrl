// SPDX-License-Identifier: GPL-2.0
/* Stand-in for /sys/class/gpio, /etc/hostname, and the status LED. */
#include "gpio_fake.hpp"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <dlfcn.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace {

constexpr const char *kLedPath = "/sys/class/leds/orangepi:red:status/brightness";
constexpr int kPass = -2;

enum FdKind {
    FdExport,
    FdUnexport,
    FdValue,
    FdDirection,
    FdEdge
};

struct FdInfo {
    FdKind kind;
    int pin;
    std::string data;
    size_t offset;
};

std::mutex g_mu;
std::unordered_map<int, FdInfo> g_fds;
std::unordered_set<int> g_exported;
std::string g_hostname = "orangepipc\n";
std::atomic<int> g_fail {FailNone};
std::atomic<int> g_irqLeft {0};
std::atomic<int> g_ledCmds {0};
std::atomic<pthread_t> g_waitingThread {};
std::atomic<int> g_waitingValid {0};

struct NoCancel {
    int old {0};
    NoCancel() { pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old); }
    ~NoCancel() { pthread_setcancelstate(old, nullptr); }
};

bool consumeFail(int kind)
{
    int expected = kind;
    return g_fail.compare_exchange_strong(expected, FailNone);
}

bool isHostname(const char *path)
{
    return path && std::strcmp(path, "/etc/hostname") == 0;
}

bool gpioSuffix(const char *path, const char *suffix, int *pin)
{
    const char *prefix = "/sys/class/gpio/gpio";
    if (!path || std::strncmp(path, prefix, std::strlen(prefix)) != 0)
        return false;

    char *end = nullptr;
    long value = std::strtol(path + std::strlen(prefix), &end, 10);
    if (!end || end == path + std::strlen(prefix) || std::strcmp(end, suffix) != 0)
        return false;

    *pin = static_cast<int>(value);
    return true;
}

int makeFd(FdKind kind, int pin, std::string data)
{
    int fd = memfd_create("gpio-fake", 0);
    if (fd < 0)
        return -1;

    std::lock_guard<std::mutex> lock(g_mu);
    g_fds[fd] = FdInfo {kind, pin, std::move(data), 0};
    return fd;
}

int hostnameFd()
{
    std::string host;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        host = g_hostname;
    }
    if (host.empty() || host.back() != '\n')
        host.push_back('\n');

    int fd = memfd_create("hostname-fake", 0);
    if (fd < 0)
        return -1;
    syscall(SYS_write, fd, host.data(), host.size());
    syscall(SYS_lseek, fd, 0, SEEK_SET);
    return fd;
}

int handlePath(const char *path)
{
    if (!path || path[0] != '/')
        return kPass;
    if (isHostname(path))
        return hostnameFd();
    if (std::strcmp(path, "/sys/class/gpio/export") == 0) {
        if (consumeFail(FailOpenExport)) {
            errno = EIO;
            return -1;
        }
        return makeFd(FdExport, -1, {});
    }
    if (std::strcmp(path, "/sys/class/gpio/unexport") == 0) {
        if (consumeFail(FailOpenUnexport)) {
            errno = EIO;
            return -1;
        }
        return makeFd(FdUnexport, -1, {});
    }

    int pin = -1;
    struct Item {
        const char *suffix;
        FdKind kind;
        int fail;
    };
    const Item items[] = {
        {"/value", FdValue, FailOpenValue},
        {"/direction", FdDirection, FailOpenDirection},
        {"/edge", FdEdge, FailOpenEdge},
    };
    for (const auto &item : items) {
        if (!gpioSuffix(path, item.suffix, &pin))
            continue;
        if (consumeFail(item.fail)) {
            errno = EIO;
            return -1;
        }
        bool exported = false;
        {
            std::lock_guard<std::mutex> lock(g_mu);
            exported = g_exported.count(pin) > 0;
        }
        if (!exported) {
            errno = ENOENT;
            return -1;
        }
        return makeFd(item.kind, pin, item.kind == FdValue ? "0" : "");
    }
    return kPass;
}

bool lookupFd(int fd, FdInfo *info)
{
    std::lock_guard<std::mutex> lock(g_mu);
    auto it = g_fds.find(fd);
    if (it == g_fds.end())
        return false;
    *info = it->second;
    return true;
}

void storeFd(int fd, const FdInfo &info)
{
    std::lock_guard<std::mutex> lock(g_mu);
    g_fds[fd] = info;
}

int openCommon(const char *path, int flags, mode_t mode)
{
    int fd = handlePath(path);
    if (fd != kPass)
        return fd;
    return static_cast<int>(syscall(SYS_openat, AT_FDCWD, path, flags, mode));
}

}  // namespace

extern "C" void test_set_hostname(const char *name)
{
    std::lock_guard<std::mutex> lock(g_mu);
    g_hostname = name ? name : "";
}

extern "C" void test_fail_next(int kind)
{
    g_fail.store(kind);
}

extern "C" void test_arm_irq(int count)
{
    g_irqLeft.store(count);
}

extern "C" int test_led_cmds(void)
{
    return g_ledCmds.load();
}

extern "C" int open(const char *path, int flags, ...)
{
    NoCancel block;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    return openCommon(path, flags, mode);
}

/* With _FILE_OFFSET_BITS=64, <fcntl.h> declares open as the open64 symbol.
   A second definition is a duplicate. */
#ifndef __USE_FILE_OFFSET64
extern "C" int open64(const char *path, int flags, ...)
{
    NoCancel block;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    return openCommon(path, flags, mode);
}
#endif

extern "C" int openat(int dirfd, const char *path, int flags, ...)
{
    NoCancel block;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    if (path && path[0] == '/')
        return openCommon(path, flags, mode);
    return static_cast<int>(syscall(SYS_openat, dirfd, path, flags, mode));
}

#ifndef __USE_FILE_OFFSET64
extern "C" int openat64(int dirfd, const char *path, int flags, ...)
{
    NoCancel block;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    if (path && path[0] == '/')
        return openCommon(path, flags, mode);
    return static_cast<int>(syscall(SYS_openat, dirfd, path, flags, mode));
}
#endif

extern "C" int access(const char *path, int mode)
{
    NoCancel block;
    int pin = -1;
    if (gpioSuffix(path, "", &pin)) {
        std::lock_guard<std::mutex> lock(g_mu);
        if (g_exported.count(pin) > 0)
            return 0;
        errno = ENOENT;
        return -1;
    }
    return static_cast<int>(syscall(SYS_faccessat, AT_FDCWD, path, mode, 0));
}

extern "C" ssize_t write(int fd, const void *buf, size_t count)
{
    NoCancel block;
    FdInfo info;
    if (!lookupFd(fd, &info))
        return syscall(SYS_write, fd, buf, count);

    std::string text(static_cast<const char *>(buf), count);
    if (info.kind == FdExport) {
        if (consumeFail(FailWriteExport)) {
            errno = EIO;
            return -1;
        }
        std::lock_guard<std::mutex> lock(g_mu);
        g_exported.insert(std::atoi(text.c_str()));
        return static_cast<ssize_t>(count);
    }
    if (info.kind == FdUnexport) {
        if (consumeFail(FailWriteUnexport)) {
            errno = EIO;
            return -1;
        }
        std::lock_guard<std::mutex> lock(g_mu);
        g_exported.erase(std::atoi(text.c_str()));
        return static_cast<ssize_t>(count);
    }
    if (info.kind == FdDirection && consumeFail(FailWriteDirection)) {
        errno = EIO;
        return -1;
    }
    if (info.kind == FdEdge && consumeFail(FailWriteEdge)) {
        errno = EIO;
        return -1;
    }
    if (info.kind == FdValue) {
        if (consumeFail(FailWriteValue)) {
            errno = EIO;
            return -1;
        }
        info.data.assign(text);
        info.offset = info.data.size();
        storeFd(fd, info);
    }
    return static_cast<ssize_t>(count);
}

extern "C" ssize_t read(int fd, void *buf, size_t count)
{
    NoCancel block;
    FdInfo info;
    if (!lookupFd(fd, &info))
        return syscall(SYS_read, fd, buf, count);
    if (info.kind == FdValue && consumeFail(FailReadValue)) {
        errno = EIO;
        return -1;
    }
    if (count == 0 || info.data.empty())
        return 0;

    /* sysfs value files return the current text from the start of each read */
    size_t start = (info.kind == FdValue) ? 0 : info.offset;
    if (start >= info.data.size())
        return 0;
    size_t n = std::min(count, info.data.size() - start);
    std::memcpy(buf, info.data.data() + start, n);
    if (info.kind != FdValue)
        info.offset = start + n;
    storeFd(fd, info);
    return static_cast<ssize_t>(n);
}

extern "C" off_t lseek(int fd, off_t offset, int whence)
{
    NoCancel block;
    FdInfo info;
    if (!lookupFd(fd, &info))
        return syscall(SYS_lseek, fd, offset, whence);
    if (consumeFail(FailLseek)) {
        errno = ESPIPE;
        return -1;
    }

    size_t base = 0;
    if (whence == SEEK_CUR)
        base = info.offset;
    else if (whence == SEEK_END)
        base = info.data.size();
    else
        base = 0;

    if (offset < 0 && static_cast<size_t>(-offset) > base) {
        errno = EINVAL;
        return -1;
    }
    info.offset = base + static_cast<size_t>(offset);
    storeFd(fd, info);
    return static_cast<off_t>(info.offset);
}

extern "C" int close(int fd)
{
    NoCancel block;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        g_fds.erase(fd);
    }
    return static_cast<int>(syscall(SYS_close, fd));
}

extern "C" int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    bool gpioFd = false;
    for (nfds_t i = 0; fds && i < nfds; i++) {
        FdInfo info;
        if (lookupFd(fds[i].fd, &info) && info.kind == FdValue)
            gpioFd = true;
    }
    if (gpioFd && g_irqLeft.load() > 0) {
        int armed = g_irqLeft.load();
        if (armed > 0)
            g_irqLeft.compare_exchange_strong(armed, armed - 1);
        int hits = 0;
        for (nfds_t i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            FdInfo info;
            if (lookupFd(fds[i].fd, &info) && info.kind == FdValue) {
                fds[i].revents = POLLPRI;
                hits++;
            }
        }
        return hits > 0 ? hits : 0;
    }

    pthread_t self = pthread_self();
    if (gpioFd) {
        g_waitingThread.store(self);
        g_waitingValid.store(1);
        pthread_testcancel();
    }
    int rc = static_cast<int>(syscall(SYS_poll, fds, nfds, timeout));
    pthread_testcancel();
    if (gpioFd && pthread_equal(g_waitingThread.load(), self))
        g_waitingValid.store(0);
    return rc;
}

static void clearWaiting(void *)
{
    pthread_t self = pthread_self();
    if (g_waitingValid.load() && pthread_equal(g_waitingThread.load(), self))
        g_waitingValid.store(0);
}

extern "C" int nanosleep(const struct timespec *req, struct timespec *rem)
{
    bool track = req && req->tv_sec == 0 && req->tv_nsec > 0 && req->tv_nsec <= 2000000L;
    pthread_t self = pthread_self();
    if (track) {
        g_waitingThread.store(self);
        g_waitingValid.store(1);
    }
    /* libc nanosleep is a cancellation point. The raw syscall is not. */
    int rc = 0;
    pthread_cleanup_push(clearWaiting, nullptr);
    pthread_testcancel();
    rc = static_cast<int>(syscall(SYS_nanosleep, req, rem));
    pthread_testcancel();
    pthread_cleanup_pop(0);
    if (track && pthread_equal(g_waitingThread.load(), self))
        g_waitingValid.store(0);
    return rc;
}

extern "C" int pthread_cancel(pthread_t thread)
{
    static int (*realCancel)(pthread_t) = nullptr;
    if (!realCancel)
        realCancel = reinterpret_cast<int (*)(pthread_t)>(dlsym(RTLD_NEXT, "pthread_cancel"));

    for (int i = 0; i < 200; i++) {
        if (g_waitingValid.load() && pthread_equal(g_waitingThread.load(), thread))
            break;
        struct timespec ts {0, 200000};
        syscall(SYS_nanosleep, &ts, nullptr);
    }
    int rc = realCancel(thread);
    for (int i = 0; i < 200; i++) {
        if (pthread_kill(thread, 0) == ESRCH)
            break;
        struct timespec ts {0, 200000};
        syscall(SYS_nanosleep, &ts, nullptr);
    }
    return rc;
}

extern "C" int stat(const char *path, struct stat *buf)
{
    if (path && std::strcmp(path, kLedPath) == 0) {
        if (buf)
            std::memset(buf, 0, sizeof(*buf));
        return 0;
    }
    /* x86_64/aarch64: newfstatat; armhf and other 32-bit: fstatat64 */
#if defined(SYS_newfstatat)
    return static_cast<int>(syscall(SYS_newfstatat, AT_FDCWD, path, buf, 0));
#elif defined(SYS_fstatat64)
    return static_cast<int>(syscall(SYS_fstatat64, AT_FDCWD, path, buf, 0));
#else
#error "no fstatat-family syscall available"
#endif
}

extern "C" int system(const char *command)
{
    if (command && std::strstr(command, "orangepi:red:status")) {
        g_ledCmds.fetch_add(1);
        return 0;
    }
    static int (*realSystem)(const char *) = nullptr;
    if (!realSystem)
        realSystem = reinterpret_cast<int (*)(const char *)>(dlsym(RTLD_NEXT, "system"));
    return realSystem(command);
}

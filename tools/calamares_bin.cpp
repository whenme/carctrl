// SPDX-License-Identifier: GPL-2.0

#include <array>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr std::string_view k_default_tty{"./tty1_ext"};

bool g_nlcr = false;
bool g_raw = true;
bool g_wait_tty = false;

int g_pid_fd = -1;
std::array<char, 1024> g_pid_file_name{};

void cleanup_pid_file();

int error_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    std::fprintf(stderr, "[calamares] ");
    const int n = std::vfprintf(stderr, format, args);
    va_end(args);
    std::fflush(nullptr);
    return n;
}

/** Read at most one chunk from fd (same semantics as original safe_read_available). */
ssize_t safe_read_available(int fd, void* buf, ssize_t count, const char* errstr)
{
    ssize_t ret = 0;
    while ((ret = ::read(fd, buf, static_cast<size_t>(count))) != 0) {
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            error_printf("%s (%s) (fd=%d)\r\n", errstr, std::strerror(errno), fd);
            break;
        }
        break;
    }
    return ret;
}

ssize_t safe_write(int fd, const void* buf, size_t count, const char* errstr)
{
    ssize_t ret = 0;
    ssize_t len = static_cast<ssize_t>(count);
    const auto* p = static_cast<const char*>(buf);

    while (len != 0 && (ret = ::write(fd, p, static_cast<size_t>(len))) != 0) {
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            error_printf("%s (%s) (fd=%d)\r\n", errstr, std::strerror(errno), fd);
            break;
        }
        len -= ret;
        p += ret;
    }

    if (ret == -1) {
        return ret;
    }
    return static_cast<ssize_t>(count) - len;
}

int forward_data(int from, int to)
{
    std::array<char, 64> buffer{};
    std::array<char, 128> buf_write{};

    const ssize_t num_read = safe_read_available(from, buffer.data(), static_cast<ssize_t>(buffer.size()),
                                                 "reading data failed");
    if (num_read == 0) {
        error_printf("no data available in fd %d, channel has closed.\n", from);
        return -1;
    }
    if (num_read < 0) {
        return -1;
    }

    std::size_t num_write = 0;
    for (ssize_t i = 0; i < num_read; ++i) {
        if (buffer[static_cast<std::size_t>(i)] == '\n') {
            if (num_write + 2 > buf_write.size()) {
                return -1;
            }
            buf_write[num_write] = '\r';
            buf_write[num_write + 1] = '\n';
            num_write += 2;
        } else {
            if (num_write + 1 > buf_write.size()) {
                return -1;
            }
            buf_write[num_write++] = buffer[static_cast<std::size_t>(i)];
        }
    }

    const ssize_t w = safe_write(to, buf_write.data(), num_write, "writing data failed");
    if (w < 0) {
        return -1;
    }
    return 0;
}

void flush_data(int from, int to)
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(from, &read_set);

    while (true) {
        fd_set rd_set = read_set;
        struct timeval timeout {};
        timeout.tv_sec = 2;

        const int num_ready = select(FD_SETSIZE, &rd_set, nullptr, nullptr, &timeout);
        if (num_ready == 0 || forward_data(from, to) < 0) {
            break;
        }
    }
}

struct termios g_orig_termios {};
int g_handlers_installed = 0;

int tty_setup(int fd);
int tty_restore(int fd);

struct sigaction g_action {};
struct sigaction g_orig_int_action {};
struct sigaction g_orig_term_action {};
struct sigaction g_orig_quit_action {};
struct sigaction g_orig_stop_action {};
struct sigaction g_orig_cont_action {};

void install_signal_handler();

extern "C" void signal_handler(int signum)
{
    switch (signum) {
    case SIGTSTP:
        (void)tty_restore(STDIN_FILENO);
        sigaction(SIGCONT, &g_action, nullptr);
        sigaction(signum, &g_orig_stop_action, nullptr);
        raise(signum);
        break;
    case SIGCONT:
        (void)tty_setup(STDIN_FILENO);
        sigaction(SIGTSTP, &g_action, nullptr);
        sigaction(signum, &g_orig_cont_action, nullptr);
        raise(signum);
        break;
    case SIGQUIT:
        cleanup_pid_file();
        (void)tty_restore(STDIN_FILENO);
        sigaction(signum, &g_orig_quit_action, nullptr);
        raise(signum);
        break;
    case SIGINT:
        cleanup_pid_file();
        (void)tty_restore(STDIN_FILENO);
        sigaction(signum, &g_orig_int_action, nullptr);
        raise(signum);
        break;
    case SIGTERM:
        cleanup_pid_file();
        (void)tty_restore(STDIN_FILENO);
        sigaction(signum, &g_orig_term_action, nullptr);
        raise(signum);
        break;
    default:
        break;
    }
}

void install_signal_handler()
{
    g_action.sa_handler = signal_handler;
    sigemptyset(&g_action.sa_mask);
    g_action.sa_flags = 0;

    sigaction(SIGINT, &g_action, &g_orig_int_action);
    sigaction(SIGTERM, &g_action, &g_orig_term_action);
    sigaction(SIGQUIT, &g_action, &g_orig_quit_action);
    sigaction(SIGTSTP, &g_action, &g_orig_stop_action);
    sigaction(SIGCONT, &g_action, &g_orig_cont_action);
}

int tty_setup(int fd)
{
    struct termios new_termios {};

    if (tcgetattr(fd, &g_orig_termios) < 0) {
        error_printf("tcgetattr failed: %s\r\n", std::strerror(errno));
        return EXIT_FAILURE;
    }
    new_termios = g_orig_termios;

    if (g_handlers_installed == 0) {
        install_signal_handler();
        g_handlers_installed = 1;
    }

    if (g_raw) {
        cfmakeraw(&new_termios);
    }

    new_termios.c_lflag |= ISIG;

    if (g_nlcr) {
        new_termios.c_oflag |= OPOST;
        new_termios.c_oflag |= ONLCR;
    }

    if (tcsetattr(fd, TCSANOW, &new_termios) < 0) {
        error_printf("tcsetattr failed: %s\r\n", std::strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int tty_restore(int fd)
{
    if (tcsetattr(fd, TCSAFLUSH, &g_orig_termios) < 0) {
        error_printf("tcsetattr failed: %s\r\n", std::strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int verify_tty_file(const char* ttyname)
{
    if (g_wait_tty == 0 && access(ttyname, F_OK) == -1) {
        error_printf("tty not available [%s]\r\n", ttyname);
        return -1;
    }

    error_printf("waiting for tty [%s] ...", ttyname);
    std::fflush(stdout);
    while (true) {
        if (access(ttyname, F_OK) != -1) {
            std::fprintf(stderr, ". ok\r\n");
            break;
        }
        std::fprintf(stderr, ".");
        sleep(1);
    }
    return 0;
}

int create_pid_file()
{
    struct flock pid_lock {};
    pid_lock.l_type = F_WRLCK;
    pid_lock.l_whence = SEEK_SET;
    pid_lock.l_start = 0;
    pid_lock.l_len = 0;

    g_pid_fd = open(g_pid_file_name.data(), O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (g_pid_fd < 0) {
        error_printf("could not open pidfile %s: %s\r\n", g_pid_file_name.data(), std::strerror(errno));
        return -1;
    }

    const int ret = fcntl(g_pid_fd, F_SETLK, &pid_lock);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EACCES) {
            error_printf("could not lock pidfile %s\r\n", g_pid_file_name.data());
        } else {
            error_printf("error lock file: %s\r\n", std::strerror(errno));
        }
        close(g_pid_fd);
        g_pid_fd = -1;
        return -1;
    }

    char pid_str[64]{};
    std::snprintf(pid_str, sizeof(pid_str), "%ld", static_cast<long>(getpid()));
    if (ftruncate(g_pid_fd, 0) < 0) {
        close(g_pid_fd);
        g_pid_fd = -1;
        return -1;
    }
    const auto len = std::strlen(pid_str);
    const ssize_t w = write(g_pid_fd, pid_str, len);
    if (w != static_cast<ssize_t>(len)) {
        error_printf("could not write to pidfile %s: %s\r\n", g_pid_file_name.data(), std::strerror(errno));
        close(g_pid_fd);
        g_pid_fd = -1;
        return -1;
    }
    error_printf("pid file locked, pid [%s]\r\n", pid_str);
    return 0;
}

void cleanup_pid_file()
{
    if (g_pid_fd != -1) {
        close(g_pid_fd);
        g_pid_fd = -1;
    }
    remove(g_pid_file_name.data());
}

int compose_tty_filename(const char* ttyname)
{
    if (verify_tty_file(ttyname) < 0) {
        return -1;
    }

    std::error_code ec;
    const std::filesystem::path canon = std::filesystem::weakly_canonical(ttyname, ec);
    if (ec) {
        error_printf("error symlink %s: %s\r\n", ttyname, ec.message().c_str());
        return -1;
    }

    std::string location = canon.string();
    for (char& ch : location) {
        if (ch == '/') {
            ch = '-';
        }
    }

    const std::string pid_path = "/var/run/calamares" + location + ".pid";
    if (pid_path.size() >= g_pid_file_name.size()) {
        error_printf("pid file path too long\r\n");
        return -1;
    }
    std::memcpy(g_pid_file_name.data(), pid_path.c_str(), pid_path.size() + 1);
    return 0;
}

int kill_existing_session(int waittime)
{
    std::ifstream file(g_pid_file_name.data());
    if (!file) {
        return 0;
    }

    int old_pid = 0;
    file >> old_pid;
    if (!file || file.fail()) {
        error_printf("parsing pidfile %s failed\r\n", g_pid_file_name.data());
        return -1;
    }

    if (::kill(old_pid, SIGTERM) != 0) {
        return -1;
    }

    while (waittime > 0) {
        if (access(g_pid_file_name.data(), R_OK) != 0) {
            return 0;
        }
        sleep(1);
        --waittime;
    }
    error_printf("timeout waiting for removal pidfile %s\r\n", g_pid_file_name.data());
    return -1;
}

void print_usage(const char* argv0)
{
    std::printf("Usage: %s [options]\n", argv0);
    std::printf("Options:\n"
                "    -h      : help (this text)\n"
                "    -t tty  : specify tty (default: %s)\n"
                "    -w      : wait for the tty to become available\n"
                "    -n      : set NL to CR-NL on output\n"
                "    -c      : use tty in canonical mode (default raw mode)\n"
                "    -f file : flush initial data to <file>\n"
                "    -k      : kill other calamares instance using this tty\n",
                k_default_tty.data());
}

int run_main(int argc, char** argv)
{
    const char* ttyname = k_default_tty.data();
    bool do_kill = false;
    const char* flush_filename = nullptr;

    opterr = 0;
    int c = 0;
    while ((c = getopt(argc, argv, "t:wdnchf:k")) != -1) {
        switch (c) {
        case 'w':
            g_wait_tty = true;
            break;
        case 't':
            ttyname = optarg;
            break;
        case 'f':
            flush_filename = optarg;
            break;
        case 'n':
            g_nlcr = true;
            break;
        case 'c':
            g_raw = false;
            break;
        case 'k':
            do_kill = true;
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        case '?':
            if (optopt == 't') {
                std::fprintf(stderr, "Option -%c requires an argument.\r\n", optopt);
            } else if (std::isprint(optopt) != 0) {
                std::fprintf(stderr, "Unknown option `-%c'.\r\n", optopt);
            } else {
                std::fprintf(stderr, "Unknown option character `\\x%x'.\r\n", optopt);
            }
            return 1;
        default:
            std::abort();
        }
    }

    if (compose_tty_filename(ttyname) < 0) {
        return EXIT_FAILURE;
    }

    if (do_kill && kill_existing_session(5) < 0) {
        return EXIT_FAILURE;
    }

    if (create_pid_file() < 0) {
        return EXIT_FAILURE;
    }

    const int ttyfd = open(ttyname, O_RDWR);
    if (ttyfd == -1) {
        error_printf("could not open tty %s: %s\r\n", ttyname, std::strerror(errno));
        return EXIT_FAILURE;
    }

    if (tty_setup(STDIN_FILENO) != EXIT_SUCCESS) {
        close(ttyfd);
        return EXIT_FAILURE;
    }

    if (flush_filename != nullptr) {
        const int flush_fd = open(flush_filename, O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC,
                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (flush_fd != -1) {
            error_printf("flushing data to [%s] ... ", flush_filename);
            flush_data(ttyfd, flush_fd);
            close(flush_fd);
            std::fprintf(stderr, "done.\r\n");
        } else {
            error_printf("flushing data to [%s] failed (%s).\r\n", flush_filename, std::strerror(errno));
        }
    }

    while (true) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(ttyfd, &read_set);
        FD_SET(STDIN_FILENO, &read_set);

        const int num_ready = select(FD_SETSIZE, &read_set, nullptr, nullptr, nullptr);
        if (num_ready > 0) {
            if (FD_ISSET(STDIN_FILENO, &read_set)) {
                if (forward_data(STDIN_FILENO, ttyfd) != 0) {
                    break;
                }
            }
            if (FD_ISSET(ttyfd, &read_set)) {
                if (forward_data(ttyfd, STDOUT_FILENO) != 0) {
                    break;
                }
            }
        }
    }

    (void)tty_restore(STDIN_FILENO);
    close(ttyfd);
    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv)
{
    return run_main(argc, argv);
}

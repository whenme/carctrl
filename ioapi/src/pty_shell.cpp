// SPDX-License-Identifier: GPL-2.0

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <cstdarg>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <exception>

#include <ioapi/pty_shell.hpp>
#include <ioapi/easylog.hpp>

struct scc_stats_reborn
{
    uint32_t pipe_continuously_full;
    uint32_t slow_serial_remainig_iterations;
    uint32_t slow_serial_entry_count;
    uint32_t dropped_chars;
    uint32_t dropped_count;
    uint32_t sent_count;
    uint32_t flushed_count;
};

static int master_fd[] = {-1, -1, -1, -1, -1};
static int rs_shell_channel = -1;

/* *** BEGIN useSTDIO support *** */
/*
 * useSTDIO tty settings support
 * NOTE: the following code is the same as used in calamares. When you change
 * something here, consider whether the same change is needed in calamares.
 */

static void usestdio_install_stop_continue_handler(void);
static void tty_restore_exit_wrapper(void);
static void tty_restore_exception_wrapper(int signum, siginfo_t *si, void *ucontext);
static struct termios orig_termios;

static int tty_setup(int fd)
{
    static int handlers_installed = 0;
    struct termios new_termios;

    if(tcgetattr(fd, &orig_termios) < 0) {
        perror("tcgetattr failed");
        return EXIT_FAILURE;
    }
    new_termios = orig_termios;

    /*
     * Before messing with the settings, install a signal handler that restores
     * the original settings. Note: only do this the first time, not after
     * every suspend/resume cycle.
     * Since some applications (like oflt) use _exit() to return, we also install
     * an 'atexit' handler (not standard atexit() because this is only run for
     * exit(), not _exit().)
     */
    if (handlers_installed == 0) {
        usestdio_install_stop_continue_handler();
        std::set_terminate(tty_restore_exit_wrapper);
        //atexception_isam(tty_restore_exception_wrapper);
        handlers_installed = 1;
    }

    /* Set raw mode: the ISAM application will handle all terminal characters */
    cfmakeraw(&new_termios);
    /* But: handle Ctrl-C / Ctrl-Z in Linux instead of ISAM (interrupt, quit, suspend) */
    new_termios.c_lflag |= ISIG;

    if (tcsetattr (fd, TCSANOW, &new_termios) < 0) {
        perror("tcsetattr failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int tty_restore(int fd)
{
    if (tcsetattr (fd, TCSAFLUSH, &orig_termios) < 0) {
        perror("tcsetattr failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void tty_restore_exit_wrapper(void)
{
    tty_restore(STDIN_FILENO);
}

static void tty_restore_exception_wrapper(int signum, siginfo_t *si, void *ucontext)
{
    tty_restore(STDIN_FILENO);
}

static struct sigaction action;
static struct sigaction orig_stop_action;
static struct sigaction orig_cont_action;

static void usestdio_stop_continue_handler(int signum)
{
    switch (signum) {
    case SIGTSTP:
        (void)tty_restore(STDIN_FILENO);
        /* Re-install our continue handler */
        sigaction(SIGCONT, &action, NULL);
        /* Call default handler */
        sigaction (signum, &orig_stop_action, NULL);
        raise(signum);
        break;

    case SIGCONT:
        (void)tty_setup(STDIN_FILENO);
        /* Re-install our stop handler */
        sigaction(SIGTSTP, &action, NULL);
        /* Call default handler */
        sigaction (signum, &orig_cont_action, NULL);
        raise(signum);
        break;

    default:
        /* Shouldn't come here */
        break;
    }
}

static void usestdio_install_stop_continue_handler(void)
{
    /*
     * We handle suspending and resuming this application
     * (SIGTSTP and SIGCONT), because we have to take care of
     * restoring the right tty settings.
     */
    action.sa_handler = usestdio_stop_continue_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction (SIGTSTP, &action, &orig_stop_action);
    sigaction (SIGCONT, &action, &orig_cont_action);
}

/* *** END useSTDIO support *** */

enum e_filetype { FT_MASTER = 1, FT_SLAVE, FT_STDIO };

static int ptym_open (int *p_master, int *p_aux, char *p_slave_name)
{
  char *ptsnam;

  *p_master=open("/dev/ptmx",O_RDWR|O_NONBLOCK);
  if(*p_master < 0)
    return 0;

  ptsnam=ptsname(*p_master);
  if(ptsnam==NULL)
  {
    close(*p_master);
    *p_master=-1;
    return 0;
  }

  grantpt(*p_master);
  unlockpt(*p_master);

  *p_aux = open(ptsnam, O_RDWR);
  if (*p_aux < 0)
    return 0;

  strcpy(p_slave_name, ptsnam);
  return 1;
}

#define MAX_TTY_DEV_NAME_LEN   64
static const char *preferred_directory = "../run.volatile";
static char tty_name_to_cleanup[MAX_TTY_DEV_NAME_LEN];

static void cleanup_tty_link()
{
    fprintf(stderr, "cleanup tty link at exit\r\n");
    apilog::warn("cleanup tty link at exit");

    if (strlen(tty_name_to_cleanup) > 0)
        unlink(tty_name_to_cleanup);
}

static void cleanup_tty_link_param(int param)
{
    cleanup_tty_link();
}

static int setup_pseudo_tty_link(uint32_t dev_num, int *return_fd, int *return_aux_fd, enum e_filetype *return_ft)
{
  char slavename[64];
  char devname[MAX_TTY_DEV_NAME_LEN] {0};
  char localname[MAX_TTY_DEV_NAME_LEN-1];
  int32_t aux_fd {-1};
  enum e_filetype filetype;
  struct termios termios;

  sprintf(localname, "scc%d", dev_num);
  master_fd[dev_num] = open(localname,O_RDWR);
  if (master_fd[dev_num] >=0)
    filetype = FT_SLAVE;
  else {
    /* Failed to open as slave ; create master */
    extern char *__progname;
    sprintf(devname, "tty_%s", __progname);
    strcpy(localname, devname);

    int32_t retVal = ptym_open (&master_fd[dev_num], &aux_fd, slavename);
    if (master_fd[dev_num] < 0 || retVal ==0) {
      printf ("scc_open() : ptym_open() failed\n");
      return -1;
    }
    filetype = FT_MASTER;

    if (master_fd[dev_num] != STDIN_FILENO) {
        /* re-direct stdio,stdout,stderr */
        dup2(master_fd[dev_num], STDIN_FILENO);
        dup2(master_fd[dev_num], STDOUT_FILENO);
        dup2(master_fd[dev_num], STDERR_FILENO);
    }
  }

  /* tell fd which process to signal */
  if (fcntl (master_fd[dev_num], F_SETOWN, getpid ()) < 0) {
    printf ("scc_open() : F_SETOWN failed\n");
    close (master_fd[dev_num]);
    if (aux_fd != -1) close(aux_fd);
    return -1;
  }

  if (tcgetattr (master_fd[dev_num], &termios) < 0) {
    perror ("ERROR getting current terminal's attributes");
    if (aux_fd != -1) close(aux_fd);
    return -1;
  }

  /* Set raw mode on tty */
  cfmakeraw(&termios);

  if (tcsetattr (master_fd[dev_num], TCSANOW, &termios) < 0) {
    perror ("ERROR setting current terminal's attributes");
    if (aux_fd != -1) close(aux_fd);
    return -1;
  }

  switch(filetype)
  {
  case FT_MASTER :
      {
        struct stat st;
        /* cleanup old tty symbolic links. */
        unlink(localname);

        /* if volatile directory exists, create symbolic link there. */
        int err = stat(preferred_directory, &st);
        if (err == 0) {
          char tmp[MAX_TTY_DEV_NAME_LEN] { 0 };
          snprintf(tmp, sizeof(tmp), "%s/%s", preferred_directory, localname);
          strcpy(localname, tmp);
        }

        if (symlink (slavename, localname) < 0) {
          printf ("scc_open() : symlink() failed\n");
          close (master_fd[dev_num]);
          if (aux_fd != -1 ) {
            close(aux_fd);
          }
          return -1;
        }
        fprintf(stderr, "pseudo_tty: %s -> %s\r\n", localname, slavename);

        strcpy(tty_name_to_cleanup, localname);
      }
      break;

  case FT_STDIO :
      fprintf(stderr,"pseudo_tty: %s -> STDIO\r\n", localname);
      break;

  case FT_SLAVE :
      fprintf(stderr,"pseudo_tty: %s -> Existing file/tty\r\n", localname);
      break;
  default:
      fprintf(stderr,"pseudo_tty: %s -> Unknown filetype %d\r\n", localname, filetype);
      break;
  }

  *return_fd = master_fd[dev_num];
  *return_aux_fd = aux_fd;
  *return_ft = filetype;

  return 0;
}

/**
 * \brief Use standard i/o or a redirection mechanism
 * If use_stdio is set to 1, input/output will be sent to the standard
 * input/output of the operating system. Otherwise, input/output will be
 * sent to a pseudo tty, that can be opened using an octopus-like tool
 */
static uint32_t use_stdio=0;
static uint32_t debug_io=0;
static uint32_t debug_tty=1;

static int error_printf(const char *format, ...)
{
    va_list args;
    int chars_printed = 0;

    va_start(args, format);
    fprintf(stderr, "[error] ");
    chars_printed = vprintf(format, args);
    va_end(args);

    fflush(NULL);

    return chars_printed;
}

/* safe read: see Linux System Programming book */
/* Only read the bytes readily available, even if less than count */
static ssize_t safe_read_available(int fd, void *buf, ssize_t count, const char *errstr)
{
    ssize_t ret = 0;

    while ((ret = read (fd, buf, count)) != 0) {
        if (ret == -1) {
            if (errno == EINTR) /* just restart */
                continue;

            /* error */
            error_printf("%s (%s) (fd=%d)\r\n", errstr, strerror(errno), fd);
            break;
        }
        break;
    }

    return ret;
}

/* safe write: see Linux System Programming book */
static ssize_t safe_write(int fd, const uint8_t *buf, size_t count, const char *errstr)
{
    ssize_t ret = 0;
    ssize_t len = count;

    while (len != 0 && (ret = write(fd, buf, len)) != 0) {
        if (ret == -1) {
            if (errno == EINTR)
                continue;

            error_printf("%s (%s) (fd=%d)\r\n", errstr, strerror(errno), fd);
            break;
        }

        len -= ret;
        buf += ret;
    }

    if (ret == -1)
        return ret;
    else
        return count - len;
}

static uint32_t scc_psos_send_buf_real_serial (uint32_t channel, uint8_t *buf, short buf_len)
{
  int fd = master_fd[channel];
  int wres = safe_write(fd, buf, buf_len, "scc_psos_send_buf_real_serial: write to fd failed");
  if (debug_tty == 1)
    fprintf(stderr, "%d = safe_write(%d,%d)\r\n", wres, fd, buf_len);

  if (wres != buf_len)
      return -1;
  else
      return 0;
}

static uint32_t scc_psos_send_buf(uint32_t channel, uint8_t *buf, short buf_len)
{
  int32_t fd;
  fd_set  fdset;
  struct  timeval timeout;
  uint32_t sleep_timeout = 0;
  int32_t retval = 0;

  static struct scc_stats_reborn scc_stats;
  static std::mutex scc_stats_mutex;

  if (debug_io == 1) {
      fprintf(stderr, "scc_psos_send_buf: writing %d bytes:", buf_len);
      for (int i = 0; i < buf_len; i++)
          fprintf(stderr, " %1$02x|%1$c", buf[i]);
      fprintf(stderr, "\r\n");
  }

  // All calls with non rs_shell_channel are redirected to a simple function.
  if (channel != (uint32_t)rs_shell_channel) {
    return scc_psos_send_buf_real_serial(channel, buf, buf_len);
  }


  /* On Linux we use a pseudo tty to simulate a serial line. The big difference with a real
   * scc is that there is no guaranteed data flow. Indeed if there is no application like
   * Calamares listening on the tty, its buffer will fill-up. Once the buffer is full, the
   * sender will be blocked. This is solved by putting the filedescriptor in non-blocking
   * mode. So the blocking issue is solved.
   *
   * Now we have the buffering aspect. If we keep on writing until the buffer is completely
   * full, part of the output (for example a part an error record) will be dropped.
   * If the reader is reading while the producer is producing it can be that (depending on
   * the speed of both) large parts of the output are corrupted because the buffer is always
   * toggling between a completely filled up state and some space available.
   * This issue is solved by using a select call on write with a timeout. The timeout value
   * is calculated based on the amount of data that needs to be printed. It is based on an
   * average speed of 8Kbitsps -> 8Kbits/8 = 1Kb/sec => so 1ms per character.
   *
   * If the time is too long, the producer task might be blocked for a too long time, resulting
   * in potential congestion issues, leading to a reset.
   *
   * If the shell is connected to a real serial line (through calamares), the serial might have
   * some hiccups. Therefore the concept of a "slow_serial" has been introduced. If a timeout is
   * seen once on the select, the slow_serial is set to a certain value. In each next call, an
   * additional wait is introduced even if the tty is ready. This to compensate the hiccups.
   *
   * Next to that also a detection for situations where no calamares is attached to the pipe.
   * A pipe is considered as not connected if the select had PIPE_FULL_COUNT_MAX times a timeout.
   * Once we end up in this state, all data is being simply dropped. Each time we enter this
   * function the select is done and once the select indicates that the FD is ready, the pipe-full
   * condition is cleared and the shell starts to send data to the pipe again.
   *
   * Note that for fast channels, like an ssh session, no delay is introduced since in practice
   * the pipe will never be full.
   *
   * The select on the (pseudo) tty has the behavior that it will report the tty is
   * full (not writable) long before it is actually full. For example the fd will not be marked
   * as writeable anymore as soon as there is 8Kbyte buffered while the actual buffer is
   * 64Kbyte. This means that in practice a write on the fd will be always successful if it
   * is indicated by the select call as ready.
   *
   * If the select returns indicating that the fd is not ready for writing, we will do the
   * write anyhow (if not in "not connected" mode), hoping that the data will fit in the queue.
   * Since the timeout is expired it means that the sending has already been stopped for some
   * time so the select just acts as a (soft) flow control mechanism in case the buffer contains
   * already >8KB data.
   */

  if (use_stdio == 1)
    fd = STDOUT_FILENO;
  else
    fd = master_fd[channel];

#define PIPE_FULL_COUNT_MAX 100

  scc_stats_mutex.lock();

  if (scc_stats.pipe_continuously_full < PIPE_FULL_COUNT_MAX) {
    timeout.tv_sec = 1 * (buf_len/1000);     /* 1ms per character */
    timeout.tv_usec = 1000 * (buf_len%1000);

    if (scc_stats.slow_serial_remainig_iterations) {
      sleep_timeout = buf_len * 1000;        /* 1ms per character */
      scc_stats.slow_serial_remainig_iterations--;
    }
  } else {
    timeout.tv_sec = timeout.tv_usec = 0;
  }

  scc_stats_mutex.unlock();

  /* unlocked section to perform the sleeps. */
  if (sleep_timeout)
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_timeout/1000));

  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  int32_t fdrdy = select(fd+1, NULL, &fdset, NULL, &timeout);
  if (debug_tty == 1) {
    fprintf(stderr, "scc_psos_send_buf: %d = select(%d) %ld:%ld\r\n", fdrdy, fd, timeout.tv_sec, timeout.tv_usec);
  }

  /* lock again */
  scc_stats_mutex.lock();

  if (fdrdy == 0) {
    /* We had a congested situation so it might mean we have a "slow" serial line. */
    scc_stats.slow_serial_remainig_iterations = 50;     /* is is an arbitrary number (so no mathematics behind it) */
    scc_stats.slow_serial_entry_count++;

    if (scc_stats.pipe_continuously_full < PIPE_FULL_COUNT_MAX) {
      scc_stats.pipe_continuously_full++;
    }
  } else {
    scc_stats.pipe_continuously_full = 0;
  }

  if (scc_stats.pipe_continuously_full < PIPE_FULL_COUNT_MAX) {
    ssize_t wres = safe_write(fd, buf, buf_len, "scc_psos_send_buf: write to fd failed");
    if (debug_tty == 1) {
      fprintf(stderr, "scc_psos_send_buf: %ld = safe_write(%d,%d)\r\n", wres, fd, buf_len);
      if (wres != buf_len)
        fprintf(stderr, "scc_psos_send_buf: %ld bytes dropped on fd %d)\r\n", buf_len-wres, fd);
    }

    /* write failed for some reason, update statistics. */
    if (wres != buf_len) {
      scc_stats.dropped_count++;
      if(wres == -1) {
        scc_stats.dropped_chars += buf_len;
      } else {
        scc_stats.dropped_chars += buf_len - wres;
        scc_stats.sent_count += wres;
      }

      retval = -1;
    } else {
      /* Send was successful, update stats */
      scc_stats.sent_count += buf_len;
    }
  } else {
    /* Pipe is continuously full, so just drop the data. */
    scc_stats.flushed_count += buf_len;
  }

  scc_stats_mutex.unlock();
  return retval;
}

static uint32_t scc_psos_recv_buf(uint32_t channel, uint8_t *buf,
                                  short buf_len, short *rcv_len)
{
  int filedesc, ret;
  fd_set set;

  if ((use_stdio == 1)&&(channel == (uint32_t)rs_shell_channel))
    filedesc = STDIN_FILENO;
  else
    filedesc = master_fd[channel];

  FD_ZERO(&set); /* clear the set */
  FD_SET(filedesc, &set); /* add our file descriptor to the set */

  do {
    ret = select(filedesc+1, &set, NULL, NULL, NULL);
    /* Retry select() when EINTR occurs */
  } while ((ret < 0) && (errno == EINTR));

  if(ret > 0)
    *rcv_len = safe_read_available(filedesc, buf, buf_len, "scc_psos_recv_buf: read after select failed");
  else  /* select timeout */
    *rcv_len = 0;

  if (debug_io == 1) {
    fprintf(stderr, "scc_psos_recv_buf: received %hd bytes:", *rcv_len);
    for (int i = 0; i < *rcv_len; i++)
      fprintf(stderr, " %1$02x|%1$c", buf[i]);
    fprintf(stderr, "\r\n");
  }

  if (*rcv_len > 0)
    return 0;
  else
    return 1;
}

void pty_shell_init(uint32_t channel)
{
  /*
   * There are two cases here:
   * - in the default case, input/output is sent to a newly created tty. This
   *   tty will be opened by a separate tool (calamares). The user reads/writes
   *   data from this tty, through that separate tool.
   *   In this case, we set raw mode directly on the tty. We don't need to handle
   *   special signals when suspend/resuming the application, as the tty is not
   *   used by anyone else like the shell.
   * - in case useSTDIO is passed on the command-line, no tty is created. All
   *   input/output is directly done on stdin/stdout. This is sometimes used
   *   during development.
   *   To get correct behavior, we need to set raw mode on stdin, just as
   *   calamares normally does. Here, we need to take care about restoring
   *   the original settings when suspending or quitting the application.
   */
  if (use_stdio == 1) {
    tty_setup(STDIN_FILENO);
  } else {
    enum e_filetype filetype;
    int real_aux_fd = -1;
    if (setup_pseudo_tty_link(channel, &master_fd[channel], &real_aux_fd, &filetype) == 0) {
      std::set_terminate(cleanup_tty_link);
      std::atexit(cleanup_tty_link);
      signal(SIGTERM, cleanup_tty_link_param);
      signal(SIGINT, cleanup_tty_link_param);
    } else {
      std::cout << "pty_shell_init: fail to setup_pseudo_tty_link..." << std::endl;
    }
  }

  rs_shell_channel = channel;
}

int putchar(int c)
{
  return (int)scc_psos_send_buf (2, (uint8_t *)(&c), 1);
}

int getchar(){
    int32_t c;
    short rcv_len;
    scc_psos_recv_buf(2, (uint8_t *)(&c), 1, &rcv_len);
    return c;
}

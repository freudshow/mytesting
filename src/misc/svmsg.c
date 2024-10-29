#include "basedef.h"
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stddef.h>                     /* For definition of offsetof() */
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define SERVER_KEY 0x1aaaaaa1           /* Key for server's message queue */

struct requestMsg { /* Requests (client to server) */
    long mtype; /* Unused */
    int clientId; /* ID of client's message queue */
    char pathname[PATH_MAX]; /* File to be returned */
};

/* REQ_MSG_SIZE computes size of 'mtext' part of 'requestMsg' structure.
 We use offsetof() to handle the possibility that there are padding
 bytes between the 'clientId' and 'pathname' fields. */

#define REQ_MSG_SIZE (offsetof(struct requestMsg, pathname) - \
                      offsetof(struct requestMsg, clientId) + PATH_MAX)

#define RESP_MSG_SIZE 8192

struct responseMsg { /* Responses (server to client) */
    long mtype; /* One of RESP_MT_* values below */
    char data[RESP_MSG_SIZE]; /* File content / response message */
};

/* Types for response messages sent from server to client */

#define RESP_MT_FAILURE 1               /* File couldn't be opened */
#define RESP_MT_DATA    2               /* Message contains file data */
#define RESP_MT_END     3               /* File data complete */

typedef enum {
    FALSE, TRUE
} Boolean;

static char *ename[] = {
/*   0 */"",
/*   1 */"EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO",
/*   7 */"E2BIG", "ENOEXEC", "EBADF", "ECHILD",
/*  11 */"EAGAIN/EWOULDBLOCK", "ENOMEM", "EACCES", "EFAULT",
/*  15 */"ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV",
/*  20 */"ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE",
/*  25 */"ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE",
/*  30 */"EROFS", "EMLINK", "EPIPE", "EDOM", "ERANGE",
/*  35 */"EDEADLK/EDEADLOCK", "ENAMETOOLONG", "ENOLCK", "ENOSYS",
/*  39 */"ENOTEMPTY", "ELOOP", "", "ENOMSG", "EIDRM", "ECHRNG",
/*  45 */"EL2NSYNC", "EL3HLT", "EL3RST", "ELNRNG", "EUNATCH",
/*  50 */"ENOCSI", "EL2HLT", "EBADE", "EBADR", "EXFULL", "ENOANO",
/*  56 */"EBADRQC", "EBADSLT", "", "EBFONT", "ENOSTR", "ENODATA",
/*  62 */"ETIME", "ENOSR", "ENONET", "ENOPKG", "EREMOTE",
/*  67 */"ENOLINK", "EADV", "ESRMNT", "ECOMM", "EPROTO",
/*  72 */"EMULTIHOP", "EDOTDOT", "EBADMSG", "EOVERFLOW",
/*  76 */"ENOTUNIQ", "EBADFD", "EREMCHG", "ELIBACC", "ELIBBAD",
/*  81 */"ELIBSCN", "ELIBMAX", "ELIBEXEC", "EILSEQ", "ERESTART",
/*  86 */"ESTRPIPE", "EUSERS", "ENOTSOCK", "EDESTADDRREQ",
/*  90 */"EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT",
/*  93 */"EPROTONOSUPPORT", "ESOCKTNOSUPPORT",
/*  95 */"EOPNOTSUPP/ENOTSUP", "EPFNOSUPPORT", "EAFNOSUPPORT",
/*  98 */"EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH",
/* 102 */"ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
/* 106 */"EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS",
/* 110 */"ETIMEDOUT", "ECONNREFUSED", "EHOSTDOWN", "EHOSTUNREACH",
/* 114 */"EALREADY", "EINPROGRESS", "ESTALE", "EUCLEAN",
/* 118 */"ENOTNAM", "ENAVAIL", "EISNAM", "EREMOTEIO", "EDQUOT",
/* 123 */"ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED", "ENOKEY",
/* 127 */"EKEYEXPIRED", "EKEYREVOKED", "EKEYREJECTED",
/* 130 */"EOWNERDEAD", "ENOTRECOVERABLE", "ERFKILL", "EHWPOISON" };

#define MAX_ENAME 133

#ifdef __GNUC__                 /* Prevent 'gcc -Wall' complaining  */
__attribute__ ((__noreturn__)) /* if we call this function as last */
#endif
static void terminate(Boolean useExit3)
{
    char *s;

    /* Dump core if EF_DUMPCORE environment variable is defined and
     is a nonempty string; otherwise call exit(3) or _exit(2),
     depending on the value of 'useExit3'. */

    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

static void outputError(Boolean useErr, int err, Boolean flushStdout, const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[3 * BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]", (err > 0 && err <= MAX_ENAME) ? ename[err] : "?UNKNOWN?", strerror(err));
    else
        snprintf(errText, BUF_SIZE, ":");

    snprintf(buf, sizeof(buf) - 1, "ERROR%s %s\n", errText, userMsg);

    if (flushStdout)
        fflush(stdout); /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr); /* In case stderr is not line-buffered */
}

void errMsg(const char *format, ...)
{
    va_list argList;
    int savedErrno;

    savedErrno = errno; /* In case we change it here */

    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);

    errno = savedErrno;
}

void errExit(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);

    terminate(TRUE);
}

static void /* SIGCHLD handler */
grimReaper(int sig)
{
    int savedErrno;

    savedErrno = errno; /* waitpid() might change 'errno' */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;
    errno = savedErrno;
}

static void /* Executed in child process: serve a single client */
serveRequest(const struct requestMsg *req)
{
    int fd;
    ssize_t numRead;
    struct responseMsg resp;

    fd = open(req->pathname, O_RDONLY);
    if (fd == -1)
    { /* Open failed: send error text */
        resp.mtype = RESP_MT_FAILURE;
        snprintf(resp.data, sizeof(resp.data), "%s", "Couldn't open");
        msgsnd(req->clientId, &resp, strlen(resp.data) + 1, 0);
        exit(EXIT_FAILURE); /* and terminate */
    }

    /* Transmit file contents in messages with type RESP_MT_DATA. We don't
     diagnose read() and msgsnd() errors since we can't notify client. */

    resp.mtype = RESP_MT_DATA;
    while ((numRead = read(fd, resp.data, RESP_MSG_SIZE)) > 0)
        if (msgsnd(req->clientId, &resp, numRead, 0) == -1)
            break;

    /* Send a message of type RESP_MT_END to signify end-of-file */

    resp.mtype = RESP_MT_END;
    msgsnd(req->clientId, &resp, 0, 0); /* Zero-length mtext */
}

int testsvmsg(int argc, char *argv[])
{
    struct requestMsg req;
    pid_t pid;
    ssize_t msgLen;
    int serverId;
    struct sigaction sa;

    /* Create server message queue */

    serverId = msgget(SERVER_KEY, IPC_CREAT | IPC_EXCL |
    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (serverId == -1)
        errExit("msgget");

    /* Establish SIGCHLD handler to reap terminated children */

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = grimReaper;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        errExit("sigaction");

    /* Read requests, handle each in a separate child process */

    for (;;)
    {
        msgLen = msgrcv(serverId, &req, REQ_MSG_SIZE, 0, 0);
        if (msgLen == -1)
        {
            if (errno == EINTR) /* Interrupted by SIGCHLD handler? */
                continue; /* ... then restart msgrcv() */
            errMsg("msgrcv"); /* Some other error */
            break; /* ... so terminate loop */
        }

        pid = fork(); /* Create child process */
        if (pid == -1)
        {
            errMsg("fork");
            break;
        }

        if (pid == 0)
        { /* Child handles request */
            serveRequest(&req);
            _exit(EXIT_SUCCESS);
        }

        /* Parent loops to receive next client request */
    }

    /* If msgrcv() or fork() fails, remove server MQ and exit */

    if (msgctl(serverId, IPC_RMID, NULL) == -1)
        errExit("msgctl");
    exit(EXIT_SUCCESS);
}

void testkey(void)
{
    int key0 = ftok("/home/floyd/repo/mytesting/db4.dat", 1);
    int key1 = ftok("/home/floyd/repo/mytesting/db4.idx", 1);
    int key2 = ftok("/home/floyd/repo/mytesting/db4.test", 1);

    printf("key0: %d, key1: %d, key2: %d\n", key0, key1, key2);
}

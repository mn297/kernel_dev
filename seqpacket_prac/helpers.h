#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

typedef struct
{
    int errnum;
    const char *name;
    const char *description;
} errno_entry;

// const errno_entry errno_list[] = {
//     {EAGAIN, "EAGAIN", "Resource temporarily unavailable"},
//     {EWOULDBLOCK, "EWOULDBLOCK", "Operation would block"},
//     {EBADF, "EBADF", "Bad file descriptor"},
//     {EFAULT, "EFAULT", "Bad address"},
//     {EFBIG, "EFBIG", "File too large"},
//     {EINTR, "EINTR", "Interrupted system call"},
//     {EINVAL, "EINVAL", "Invalid argument"},
//     {EIO, "EIO", "Input/output error"},
//     {ENOSPC, "ENOSPC", "No space left on device"},
//     {EPIPE, "EPIPE", "Broken pipe"},
//     {EDEADLK, "EDEADLK", "Resource deadlock avoided"},
//     {ENOLCK, "ENOLCK", "No locks available"},
//     // Add more errno cases as needed
//     {0, NULL, NULL} // Sentinel value to mark end of array
// };

const errno_entry errno_list[] = {
    {E2BIG, "E2BIG", "Argument list too long"},
    {EACCES, "EACCES", "Permission denied"},
    {EADDRINUSE, "EADDRINUSE", "Address already in use"},
    {EADDRNOTAVAIL, "EADDRNOTAVAIL", "Cannot assign requested address"},
    {EAFNOSUPPORT, "EAFNOSUPPORT", "Address family not supported by protocol"},
    {EAGAIN, "EAGAIN", "Resource temporarily unavailable"},
    {EALREADY, "EALREADY", "Operation already in progress"},
    {EBADF, "EBADF", "Bad file descriptor"},
    {EBADMSG, "EBADMSG", "Bad message"},
    {EBUSY, "EBUSY", "Device or resource busy"},
    {ECANCELED, "ECANCELED", "Operation canceled"},
    {ECHILD, "ECHILD", "No child processes"},
    {ECONNABORTED, "ECONNABORTED", "Connection aborted"},
    {ECONNREFUSED, "ECONNREFUSED", "Connection refused"},
    {ECONNRESET, "ECONNRESET", "Connection reset by peer"},
    {EDEADLK, "EDEADLK", "Resource deadlock avoided"},
    {EDESTADDRREQ, "EDESTADDRREQ", "Destination address required"},
    {EDOM, "EDOM", "Mathematics argument out of domain of function"},
    {EDQUOT, "EDQUOT", "Disk quota exceeded"},
    {EEXIST, "EEXIST", "File exists"},
    {EFAULT, "EFAULT", "Bad address"},
    {EFBIG, "EFBIG", "File too large"},
    {EHOSTUNREACH, "EHOSTUNREACH", "No route to host"},
    {EIDRM, "EIDRM", "Identifier removed"},
    {EILSEQ, "EILSEQ", "Invalid or incomplete multibyte or wide character"},
    {EINPROGRESS, "EINPROGRESS", "Operation now in progress"},
    {EINTR, "EINTR", "Interrupted system call"},
    {EINVAL, "EINVAL", "Invalid argument"},
    {EIO, "EIO", "Input/output error"},
    {EISCONN, "EISCONN", "Transport endpoint is already connected"},
    {EISDIR, "EISDIR", "Is a directory"},
    {ELOOP, "ELOOP", "Too many levels of symbolic links"},
    {EMFILE, "EMFILE", "Too many open files"},
    {EMLINK, "EMLINK", "Too many links"},
    {EMSGSIZE, "EMSGSIZE", "Message too long"},
    {EMULTIHOP, "EMULTIHOP", "Multihop attempted"},
    {ENAMETOOLONG, "ENAMETOOLONG", "File name too long"},
    {ENETDOWN, "ENETDOWN", "Network is down"},
    {ENETRESET, "ENETRESET", "Network dropped connection on reset"},
    {ENETUNREACH, "ENETUNREACH", "Network is unreachable"},
    {ENFILE, "ENFILE", "File table overflow"},
    {ENOBUFS, "ENOBUFS", "No buffer space available"},
    {ENODATA, "ENODATA", "No data available"},
    {ENODEV, "ENODEV", "No such device"},
    {ENOENT, "ENOENT", "No such file or directory"},
    {ENOEXEC, "ENOEXEC", "Exec format error"},
    {ENOLCK, "ENOLCK", "No locks available"},
    {ENOLINK, "ENOLINK", "Link has been severed"},
    {ENOMEM, "ENOMEM", "Cannot allocate memory"},
    {ENOMSG, "ENOMSG", "No message of desired type"},
    {ENOPROTOOPT, "ENOPROTOOPT", "Protocol not available"},
    {ENOSPC, "ENOSPC", "No space left on device"},
    {ENOSR, "ENOSR", "Out of streams resources"},
    {ENOSTR, "ENOSTR", "Device not a stream"},
    {ENOSYS, "ENOSYS", "Function not implemented"},
    {ENOTCONN, "ENOTCONN", "Transport endpoint is not connected"},
    {ENOTDIR, "ENOTDIR", "Not a directory"},
    {ENOTEMPTY, "ENOTEMPTY", "Directory not empty"},
    {ENOTRECOVERABLE, "ENOTRECOVERABLE", "State not recoverable"},
    {ENOTSOCK, "ENOTSOCK", "Socket operation on non-socket"},
    {ENOTSUP, "ENOTSUP", "Operation not supported"},
    {ENOTTY, "ENOTTY", "Inappropriate ioctl for device"},
    {ENXIO, "ENXIO", "No such device or address"},
    {EOPNOTSUPP, "EOPNOTSUPP", "Operation not supported on transport endpoint"},
    {EOVERFLOW, "EOVERFLOW", "Value too large for defined data type"},
    {EOWNERDEAD, "EOWNERDEAD", "Owner died"},
    {EPERM, "EPERM", "Operation not permitted"},
    {EPFNOSUPPORT, "EPFNOSUPPORT", "Protocol family not supported"},
    {EPIPE, "EPIPE", "Broken pipe"},
    {EPROTO, "EPROTO", "Protocol error"},
    {EPROTONOSUPPORT, "EPROTONOSUPPORT", "Protocol not supported"},
    {EPROTOTYPE, "EPROTOTYPE", "Protocol wrong type for socket"},
    {ERANGE, "ERANGE", "Result too large"},
    {EROFS, "EROFS", "Read-only file system"},
    {ESPIPE, "ESPIPE", "Invalid seek"},
    {ESRCH, "ESRCH", "No such process"},
    {ESTALE, "ESTALE", "Stale file handle"},
    {ETIME, "ETIME", "Timer expired"},
    {ETIMEDOUT, "ETIMEDOUT", "Connection timed out"},
    {ETXTBSY, "ETXTBSY", "Text file busy"},
    {EWOULDBLOCK, "EWOULDBLOCK", "Operation would block"},
    {EXDEV, "EXDEV", "Cross-device link"},
    {0, NULL, NULL} // Sentinel value to mark end of array
};
const char *errno_to_name(int err, const char **description)
{
    for (const errno_entry *entry = errno_list; entry->name != NULL; ++entry)
    {
        if (entry->errnum == err)
        {
            if (description)
            {
                *description = entry->description;
            }
            return entry->name;
        }
    }
    if (description)
    {
        *description = "Unknown error";
    }
    return "UNKNOWN_ERRNO";
}

long get_inode(int fd)
{
    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1)
    {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    // printf("Inode number: %ld\n", (long)statbuf.st_ino);
    return (long)statbuf.st_ino;
}

void get_socket_info()
{
    printf("=== Socket Information ===\n");
    char command[BUFFER_SIZE];

    // Get the socket details using lsof
    snprintf(command, sizeof(command), "lsof -U | grep %s", SOCKET_PATH);
    printf("Running command: %s\n", command);
    system(command);

    // Get the socket details using ss
    snprintf(command, sizeof(command), "ss -xl | grep %s", SOCKET_PATH);
    printf("Running command: %s\n", command);
    system(command);

    printf("==========================\n");
}
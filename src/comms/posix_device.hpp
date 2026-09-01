#pragma once

#include <cerrno>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace drivers::comms::detail {

/// Opens `path` as a character device. Rejects empty paths, paths outside
/// `/dev/`, and anything that does not resolve to a character device node
/// (e.g. a regular file or FIFO substituted for the real device node).
/// Returns -1 with `errno` set on any failure; never leaks the fd.
inline int openCharDevice(const std::string& path, int extra_flags) noexcept {
    constexpr std::string_view kDevPrefix = "/dev/";
    if (path.compare(0, kDevPrefix.size(), kDevPrefix) != 0) {
        errno = EINVAL;
        return -1;
    }

    const int fd = ::open(path.c_str(), O_CLOEXEC | extra_flags);
    if (fd < 0)
        return -1;

    struct stat st {};
    if (::fstat(fd, &st) != 0) {
        const int saved_errno = errno;
        ::close(fd);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISCHR(st.st_mode)) {
        ::close(fd);
        errno = ENOTTY;
        return -1;
    }
    return fd;
}

} // namespace drivers::comms::detail

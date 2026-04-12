// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <cmn/bit_manip.h>
#include <types/concepts.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#else
#error "Unsupported OS for file I/O operations"
#endif

namespace cmn::io
{

/** @return File descriptor for standard input
 */
[[nodiscard]] inline int stdinFileno()
{
#if defined(__linux__)
    return STDIN_FILENO;
#elif defined(_WIN32)
    return ::_fileno(stdin);
#else
    return -1;
#endif
}

/** @return File descriptor for standard output
 */
[[nodiscard]] inline int stdoutFileno()
{
#if defined(__linux__)
    return STDOUT_FILENO;
#elif defined(_WIN32)
    return ::_fileno(stdout);
#else
    return -1;
#endif
}

/** @return File descriptor for standard error
 */
[[nodiscard]] inline int stderrFileno()
{
#if defined(__linux__)
    return STDERR_FILENO;
#elif defined(_WIN32)
    return ::_fileno(stderr);
#else
    return -1;
#endif
}

/** Write data to a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to write to
 * @param buffer Data to write
 * @param size Number of bytes to write
 *
 * @return Number of bytes written, or -1 on error
 */
inline intmax_t write(int fileDescriptor, const void* buffer, size_t size)
{
#if defined(__linux__)
    return ::write(fileDescriptor, buffer, size);
#elif defined(_WIN32)
    return ::_write(fileDescriptor, buffer, tng::cmn::narrow<unsigned int>(size));
#else
    return -1;
#endif
}

/** Write data to a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to write to
 * @param buffer Data to write
 *
 * @return Number of bytes written, or -1 on error
 */
template<cmn::TrivialStandardLayoutType ValueType>
intmax_t write(int fileDescriptor, std::span<const ValueType> buffer)
{
    return write(fileDescriptor, buffer.data(), buffer.size_bytes());
}

/** Write a string to a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to write to
 * @param buffer String to write
 *
 * @return Number of bytes written, or -1 on error
 */
inline intmax_t write(int fileDescriptor, std::string_view buffer)
{
    return write(fileDescriptor, std::span{buffer});
}

/** Read data from a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to read from
 * @param buffer Buffer to store the read data
 * @param size Number of bytes to read
 *
 * @return Number of bytes read, or -1 on error
 */
inline intmax_t read(int fileDescriptor, void* buffer, size_t size)
{
#if defined(__linux__)
    return ::read(fileDescriptor, buffer, size);
#elif defined(_WIN32)
    return ::_read(fileDescriptor, buffer, tng::cmn::narrow<unsigned int>(size));
#else
    return -1;
#endif
}

/** Read data from a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to read from
 * @param buffer Buffer to store the read data
 *
 * @return Number of bytes read, or -1 on error
 */
template<tng::cmn::TrivialStandardLayoutType ValueType>
intmax_t read(int fileDescriptor, std::span<ValueType> buffer)
{
    return read(fileDescriptor, buffer.data(), buffer.size_bytes());
}

/** Flush a file specified by a descriptor
 *
 * @param fileDescriptor Descriptor for the file to flush
 *
 * @return 0 on success, or -1 on error
 */
inline int flush(int fileDescriptor)
{
#if defined(__linux__)
    return ::fsync(fileDescriptor);
#elif defined(_WIN32)
    return ::_commit(fileDescriptor);
#else
    return -1;
#endif
}

}  // namespace cmn::io

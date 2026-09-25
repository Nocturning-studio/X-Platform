/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once

#include <stdexcept>
#include <string>
#include "LibInternal.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Exception : public std::runtime_error
{
public:
    explicit Exception(const std::string& message) : std::runtime_error(message) {}
};

/// Thrown when the device or context is in an invalid state for an operation.
/// Example: missing vertex shader, empty index buffer, zero viewport.
class InvalidState : public Exception
{
public:
    explicit InvalidState(const std::string& message) : Exception("Invalid state: " + message) {}
};

/// Thrown when a function argument is out of allowed range or invalid.
class InvalidArgument : public Exception
{
public:
    explicit InvalidArgument(const std::string& message) : Exception("Invalid argument: " + message) {}
};

/// Thrown when a platform-specific operation fails.
/// Example: cannot create console buffer, GDI call fails.
class PlatformError : public Exception
{
public:
    explicit PlatformError(const std::string& message) : Exception("Platform error: " + message) {}
};

/// Thrown when a requested feature is not yet implemented on the current platform.
class NotImplemented : public Exception
{
public:
    explicit NotImplemented(const std::string& feature) : Exception("Not implemented: " + feature) {}
};

/// Thrown on file I/O errors (TGA save, etc.).
class IOError : public Exception
{
public:
    explicit IOError(const std::string& message) : Exception("I/O error: " + message) {}
};

/// Thrown when a memory allocation fails unexpectedly.
class OutOfMemory : public Exception
{
public:
    explicit OutOfMemory(const std::string& message) : Exception("Out of memory: " + message) {}
};

SOFTX_END
/////////////////////////////////////////////////////////////////
#ifdef NDEBUG
    #define SOFTX_THROW(exc) throw exc
#else
    #define SOFTX_THROW(exc) \
        do { \
            std::cerr << "SOFTX_THROW: " << (exc).what() << std::endl; \
            std::abort(); \
        } while(false)
#endif
/////////////////////////////////////////////////////////////////

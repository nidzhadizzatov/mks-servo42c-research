#include "mks_servo42c/serial_port.hpp"

#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <errno.h>
#endif

namespace mks_servo42c {

struct SerialPort::Impl {
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif
};

SerialPort::SerialPort() : impl_(new Impl()) {}

SerialPort::~SerialPort() {
    close();
    delete impl_;
}

SerialPort::SerialPort(SerialPort&& other) noexcept : impl_(other.impl_) {
    other.impl_ = nullptr;
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {
    if (this != &other) {
        close();
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

bool SerialPort::open(const std::string& port_name, int baudrate) {
    close();

#ifdef _WIN32
    // Windows
    std::string full_name = "\\\\.\\" + port_name;
    impl_->handle = CreateFileA(
        full_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (impl_->handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(impl_->handle, &dcb)) {
        CloseHandle(impl_->handle);
        impl_->handle = INVALID_HANDLE_VALUE;
        return false;
    }

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(impl_->handle, &dcb)) {
        CloseHandle(impl_->handle);
        impl_->handle = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(impl_->handle, &timeouts);

    return true;

#else
    // Linux/macOS
    impl_->fd = ::open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (impl_->fd < 0) {
        return false;
    }

    struct termios tty = {};
    if (tcgetattr(impl_->fd, &tty) != 0) {
        ::close(impl_->fd);
        impl_->fd = -1;
        return false;
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(impl_->fd, TCSANOW, &tty) != 0) {
        ::close(impl_->fd);
        impl_->fd = -1;
        return false;
    }

    return true;
#endif
}

void SerialPort::close() {
    if (!impl_) return;

#ifdef _WIN32
    if (impl_->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(impl_->handle);
        impl_->handle = INVALID_HANDLE_VALUE;
    }
#else
    if (impl_->fd >= 0) {
        ::close(impl_->fd);
        impl_->fd = -1;
    }
#endif
}

bool SerialPort::is_open() const {
    if (!impl_) return false;
#ifdef _WIN32
    return impl_->handle != INVALID_HANDLE_VALUE;
#else
    return impl_->fd >= 0;
#endif
}

size_t SerialPort::write(const uint8_t* data, size_t length) {
    if (!is_open()) return 0;

#ifdef _WIN32
    DWORD written = 0;
    if (!WriteFile(impl_->handle, data, static_cast<DWORD>(length),
                   &written, nullptr)) {
        return 0;
    }
    return written;
#else
    ssize_t n = ::write(impl_->fd, data, length);
    return (n < 0) ? 0 : static_cast<size_t>(n);
#endif
}

size_t SerialPort::read(uint8_t* buffer, size_t max_length, int timeout_ms) {
    if (!is_open()) return 0;

#ifdef _WIN32
    DWORD read_bytes = 0;
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = timeout_ms;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    SetCommTimeouts(impl_->handle, &timeouts);

    if (!ReadFile(impl_->handle, buffer, static_cast<DWORD>(max_length),
                  &read_bytes, nullptr)) {
        return 0;
    }
    return read_bytes;
#else
    // Используем poll для таймаута
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(impl_->fd, &read_fds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = ::select(impl_->fd + 1, &read_fds, nullptr, nullptr, &tv);
    if (ret <= 0) return 0;

    ssize_t n = ::read(impl_->fd, buffer, max_length);
    return (n < 0) ? 0 : static_cast<size_t>(n);
#endif
}

void SerialPort::flush_input() {
    if (!is_open()) return;
#ifdef _WIN32
    PurgeComm(impl_->handle, PURGE_RXCLEAR);
#else
    tcflush(impl_->fd, TCIFLUSH);
#endif
}

void SerialPort::flush_output() {
    if (!is_open()) return;
#ifdef _WIN32
    PurgeComm(impl_->handle, PURGE_TXCLEAR);
#else
    tcflush(impl_->fd, TCOFLUSH);
#endif
}

}  // namespace mks_servo42c
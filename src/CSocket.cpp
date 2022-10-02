#include <CSocket.h>
#include <COSRead.h>
#include <CFile.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

CSocket::
CSocket(const std::string &name) :
 name_(name)
{
  if (name_.substr(0, 2) == "//") {
    domain_ = AF_INET;
    name_   = name_.substr(2);
  }
}

CSocket::
~CSocket()
{
  if (fd_ != 0)
    close();
}

bool
CSocket::
openClient()
{
  is_server_ = false;

  return openClientServer();
}

bool
CSocket::
openServer()
{
  is_server_ = true;

  return openClientServer();
}

bool
CSocket::
openClientServer()
{
  if (is_server_ && domain_ == AF_UNIX)
    CFile::remove(name_);

  struct sockaddr_storage sa;
  socklen_t               sa_len;

  if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len,
                       name_.c_str(), domain_, is_server_))
    return false;

  int protocol = 0;

  fd_ = ::socket(domain_, SOCK_STREAM, protocol);

  if (fd_ < 0)
    return false;

  setFdHWM(fd_);

  if (domain_ == AF_INET) {
    int socket_option_value = 1;

    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                   &socket_option_value, sizeof(socket_option_value)) != 0)
      return false;
  }

  if (is_server_) {
    FD_ZERO(&fd_set_);

    if (bind(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) != 0)
      return false;

    if (listen(fd_, SOMAXCONN) != 0)
      return false;

    FD_SET(fd_, &fd_set_);
  }
  else {
    if (connect(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) != 0)
      return false;
  }

  return true;
}

bool
CSocket::
waitServer()
{
  if (! is_server_) {
    errno = ENOTSUP;
    return false;
  }

  fd_client_ = -1;

  while (true) {
    fd_set fd_set_read = fd_set_;

    if (select(fd_hwm_ + 1, &fd_set_read, nullptr, nullptr, nullptr) < 0)
      return false;

    for (int fd = 0; fd <= fd_hwm_; ++fd) {
      if (! FD_ISSET(fd, &fd_set_read))
        continue;

      if (fd == fd_) {
        struct sockaddr_un from;
        socklen_t          from_len = sizeof(from);

        int clientfd = accept(fd, reinterpret_cast<struct sockaddr *>(&from), &from_len);

        if (clientfd < 0)
          return false;

        FD_SET(clientfd, &fd_set_);

        setFdHWM(clientfd);

        continue;
      }

      fd_client_ = fd;

      return true;
    }
  }
}

bool
CSocket::
writeServer(const std::string &str)
{
  return writeServer(str.c_str(), str.size());
}

bool
CSocket::
writeServer(const char *data, size_t size)
{
  //TODO: use send
  return COSRead::writeall(fd_, data, size);
}

bool
CSocket::
writeClient(const std::string &str)
{
  return writeClient(str.c_str(), str.size());
}

bool
CSocket::
writeClient(const char *data, size_t size)
{
  if (fd_client_ < 0)
    return false;

  //TODO: use send
  return COSRead::writeall(fd_client_, data, size);
}

bool
CSocket::
readServer(std::string &str)
{
  int  nread;
  char buffer[512];

  if (! readServer(buffer, sizeof(buffer), &nread))
    return false;

  if (nread == -1)
    return false;

  str = std::string(buffer, size_t(nread));

  return true;
}

bool
CSocket::
readServer(char *data, size_t size, int *nread)
{
  //TODO: use recv
  *nread = int(::read(fd_, data, size));

  if (*nread < 0)
    return false;

  return true;
}

bool
CSocket::
readClient(std::string &str)
{
  int  nread;
  char buffer[512];

  if (! readClient(buffer, sizeof(buffer), &nread))
    return false;

  str = std::string(buffer, size_t(nread));

  return true;
}

bool
CSocket::
readClient(char *data, size_t size, int *nread)
{
  if (fd_client_ < 0)
    return false;

  //TODO: use recv
  *nread = int(::read(fd_client_, data, size));

  if (*nread < 0)
    return false;

  return true;
}

bool
CSocket::
close()
{
  if (is_server_) {
    for (int fd = 0; fd <= fd_hwm_; ++fd)
      if (FD_ISSET(fd, &fd_set_))
        ::close(fd);

    if (domain_ == AF_UNIX)
      CFile::remove(name_);
  }
  else
    ::close(fd_);

  fd_ = -1;

  return true;
}

bool
CSocket::
closeClient()
{
  if (fd_client_ < 0)
    return false;

  ::close(fd_client_);

  FD_CLR(fd_client_, &fd_set_);

  resetFdHWM(fd_client_);

  fd_client_ = -1;

  return true;
}

void
CSocket::
setFdHWM(int fd)
{
  fd_hwm_ = std::max(fd_hwm_, fd);
}

void
CSocket::
resetFdHWM(int fd)
{
  if (fd == fd_hwm_)
    --fd_hwm_;
}

/*TODO: Move to COS */
/*TODO: Add auto_free class */
bool
CSocket::
makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *name, int domain, bool will_bind)
{
  if (domain == AF_UNIX) {
    struct sockaddr_un *sun = reinterpret_cast<struct sockaddr_un *>(sa);

    if (strlen(name) >= sizeof(sun->sun_path)) {
      errno = ENAMETOOLONG;
      return false;
    }

    memset(sun, 0, sizeof(*sun));

    sun->sun_family = AF_UNIX;

    strncpy(sun->sun_path, name, sizeof(sun->sun_path) - 1);

    *len = sizeof(*sun);
  }
  else {
    struct addrinfo hint;

    memset(&hint, 0, sizeof(hint));

    hint.ai_family   = domain;
    hint.ai_socktype = SOCK_STREAM;

    if (will_bind)
      hint.ai_flags = AI_PASSIVE;

    char *nodename = strdup(name);

    // <host>:<port>
    char *pc = strchr(nodename, ':');

    if (pc == nullptr) {
      free(nodename);
      errno = EINVAL;
      return false;
    }

    *pc = '\0';

    const char *portName = pc + 1;

    struct addrinfo *info = nullptr;

    if (getaddrinfo(nodename, portName, &hint, &info) != 0) {
      free(nodename);
      freeaddrinfo(info);
      errno = EINVAL;
      return false;
    }

    free(nodename);

    memcpy(sa, info->ai_addr, info->ai_addrlen);

    *len = info->ai_addrlen;

    freeaddrinfo(info);
  }

  return true;
}

int
CSocket::
getFreePort(int startPort)
{
  int socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket == -1) return -1;

  std::string errMsg;

  bool connected = false;

  uint16_t port = uint16_t(startPort);

  while (! connected) {
    bool rc = true;

    struct sockaddr_in sin;

    sin.sin_port        = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_family      = AF_INET;

    if (bind(socket, reinterpret_cast<struct sockaddr *>(&sin), sizeof(struct sockaddr_in)) == -1) {
      if (errno == EADDRINUSE)
        errMsg = "Port in use";
      else
        errMsg = "Bind failed";
      rc = false;
    }

    if (rc) {
      socklen_t len = sizeof(sin);

      if (getsockname(socket, reinterpret_cast<struct sockaddr *>(&sin), &len) == -1) {
        errMsg = "Failed to get socket name";
        rc = false;
      }
    }

    if (rc) {
      port = ntohs(sin.sin_port);

      ::close(socket);

      connected = true;

      break;
    }

    if (startPort == 0)
      break;

    ++port;
  }

  if (! connected) {
    std::cerr << errMsg << "\n";
    return -1;
  }

  return port;
}

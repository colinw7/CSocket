#include <CUnixSocket.h>
#include <CFile.h>
#include <CStrUtil.h>
#include <COSRead.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

CUnixSocket::
CUnixSocket(const std::string &name) :
 name_(name)
{
}

CUnixSocket::
~CUnixSocket()
{
  if (fd_)
    close();
}

bool
CUnixSocket::
openClient()
{
  is_server_ = false;

  return openClientServer();
}

bool
CUnixSocket::
openServer()
{
  is_server_ = true;

  return openClientServer();
}

bool
CUnixSocket::
openClientServer()
{
  if (is_server_)
    CFile::remove(name_);

  struct sockaddr_storage sa;
  socklen_t               sa_len;

  if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len,
                       name_.c_str(), abstract_))
    return false;

  if (isDatagram())
    fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  else
    fd_ = ::socket(AF_UNIX, SOCK_DGRAM , 0);

  if (fd_ < 0)
    return false;

  setFdHWM(fd_);

  if (is_server_) {
    FD_ZERO(&fd_set_);

    if (bind(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) == -1)
      return false;

    if (! isDatagram()) {
      if (listen(fd_, SOMAXCONN) != 0)
        return false;
    }

    FD_SET(fd_, &fd_set_);
  }
  else {
    if (! isDatagram()) {
      if (connect(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) != 0)
        return false;
    }
    else {
      std::string clientName = name_ + "_cl." + CStrUtil::toString(getpid());

      if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len,
                           clientName.c_str(), abstract_))
        return false;

      if (bind(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) == -1)
        return false;
    }
  }

  return true;
}

// wait for server to connect to client
bool
CUnixSocket::
waitServer()
{
  if (! is_server_) {
    errno = ENOTSUP;
    return false;
  }

  if (isDatagram())
    return true;

  fd_client_ = -1;

  while (true) {
    fd_set fd_set_read = fd_set_;

    if (::select(fd_hwm_ + 1, &fd_set_read, nullptr, nullptr, nullptr) < 0)
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

// client writes data to server
bool
CUnixSocket::
writeServer(const std::string &str)
{
  return writeServer(str.c_str(), str.size());
}

// client writes data to server
bool
CUnixSocket::
writeServer(const char *data, size_t size)
{
  if (! isDatagram()) {
    //TODO: use send
    return COSRead::writeall(fd_, data, size);
  }
  else {
    struct sockaddr_un claddr;

    socklen_t len = sizeof(struct sockaddr_un);

    auto len1 = sendto(fd_, data, size, 0, reinterpret_cast<struct sockaddr *>(&claddr), len);
    if (len1 < 0) return false;

    return (size_t(len1) == size);
  }
}

// server writes data to client
bool
CUnixSocket::
writeClient(const std::string &str)
{
  return writeClient(str.c_str(), str.size());
}

// server writes data to client
bool
CUnixSocket::
writeClient(const char *data, size_t size)
{
  if (fd_client_ < 0)
    return false;

  if (! isDatagram()) {
    //TODO: use send
    return COSRead::writeall(fd_client_, data, size);
  }
  else {
    struct sockaddr_un claddr;

    socklen_t len = sizeof(struct sockaddr_un);

    auto len1 = sendto(fd_, data, size, 0, reinterpret_cast<struct sockaddr *>(&claddr), len);
    if (len1 < 0) return false;

    return (size_t(len1) == size);
  }
}

// client reads data from server
bool
CUnixSocket::
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

// client reads data from server
bool
CUnixSocket::
readServer(char *data, size_t size, int *nread)
{
  if (! isDatagram()) {
    //TODO: use recv
    *nread = int(::read(fd_, data, size));

    if (*nread < 0)
      return false;
  }
  else {
    struct sockaddr_un claddr;

    socklen_t len = sizeof(struct sockaddr_un);

    *nread = int(::recvfrom(fd_, data, size, 0,
                            reinterpret_cast<struct sockaddr *>(&claddr), &len));

    if (*nread < 0)
      return false;
  }

  return true;
}

// server reads data from client
bool
CUnixSocket::
readClient()
{
  std::string str;

  if (! readClient(str))
    return false;

  handleRequest(str);

  return true;
}

// server reads data from client
bool
CUnixSocket::
readClient(std::string &str)
{
  int  nread;
  char buffer[512];

  if (! readClient(buffer, sizeof(buffer), &nread))
    return false;

  str = std::string(buffer, size_t(nread));

  return true;
}

// server reads data from client
bool
CUnixSocket::
readClient(char *data, size_t size, int *nread)
{
  if (fd_client_ < 0)
    return false;

  if (! isDatagram()) {
    //TODO: use recv
    *nread = int(::read(fd_client_, data, size));

    if (*nread < 0)
      return false;
  }
  else {
    struct sockaddr_un claddr;

    socklen_t len = sizeof(struct sockaddr_un);

    *nread = int(::recvfrom(fd_, data, size, 0,
                            reinterpret_cast<struct sockaddr *>(&claddr), &len));

    if (*nread < 0)
      return false;
  }

  return true;
}

bool
CUnixSocket::
close()
{
  if (is_server_) {
    for (int fd = 0; fd <= fd_hwm_; ++fd)
      if (FD_ISSET(fd, &fd_set_))
        ::close(fd);

    CFile::remove(name_);
  }
  else
    ::close(fd_);

  fd_ = -1;

  return true;
}

bool
CUnixSocket::
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
CUnixSocket::
setFdHWM(int fd)
{
  fd_hwm_ = std::max(fd_hwm_, fd);
}

void
CUnixSocket::
resetFdHWM(int fd)
{
  if (fd == fd_hwm_)
    --fd_hwm_;
}

/*TODO: Move to COS */
/*TODO: Add auto_free class */
bool
CUnixSocket::
makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *name, bool abstract)
{
  auto *sun = reinterpret_cast<struct sockaddr_un *>(sa);

  if (strlen(name) >= sizeof(sun->sun_path)) {
    errno = ENAMETOOLONG;
    return false;
  }

  memset(sun, 0, sizeof(*sun));

  sun->sun_family = AF_UNIX;

  if (! abstract)
    strncpy(sun->sun_path, name, sizeof(sun->sun_path) - 1);
  else {
    sun->sun_path[0] = '\0';

    strncpy(&sun->sun_path[1], name, sizeof(sun->sun_path) - 2);
  }

  *len = sizeof(*sun);

  return true;
}

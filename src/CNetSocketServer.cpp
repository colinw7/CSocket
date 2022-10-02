#include <CNetSocketServer.h>
#include <CFile.h>
#include <CAutoFree.h>
#include <COSRead.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

CNetSocketServer::
CNetSocketServer(const std::string &hostname) :
 hostname_(hostname)
{
}

CNetSocketServer::
~CNetSocketServer()
{
  close();
}

bool
CNetSocketServer::
open()
{
  struct sockaddr_storage sa;
  socklen_t               sa_len;

  if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len, hostname_.c_str()))
    return false;

  //----

  // Open socket

  int protocol = 0; // IPPROTO_TCP

  fd_ = ::socket(AF_INET, SOCK_STREAM, protocol);

  if (fd_ < 0)
    return false;

  setFdHWM(fd_);

  //----

  int socket_option_value = 1;

  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                 &socket_option_value, sizeof(socket_option_value)) != 0)
    return false;

  //----

  FD_ZERO(&fd_set_);

  if (bind(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) != 0)
    return false;

  if (listen(fd_, SOMAXCONN) != 0)
    return false;

  FD_SET(fd_, &fd_set_);

  return true;
}

bool
CNetSocketServer::
wait()
{
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
CNetSocketServer::
read(std::string &str)
{
  int  nread;
  char buffer[512];

  if (! read(buffer, sizeof(buffer), &nread))
    return false;

  str = std::string(buffer, size_t(nread));

  return true;
}

bool
CNetSocketServer::
read(char *data, size_t size, int *nread)
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
CNetSocketServer::
write(const std::string &str)
{
  return write(str.c_str(), str.size());
}

bool
CNetSocketServer::
write(const char *data, size_t size)
{
  if (fd_client_ < 0)
    return false;

  //TODO: use send
  return COSRead::writeall(fd_client_, data, size);
}

bool
CNetSocketServer::
close()
{
  for (int fd = 0; fd <= fd_hwm_; ++fd)
    if (FD_ISSET(fd, &fd_set_))
      ::close(fd);

  fd_ = -1;

  return true;
}

bool
CNetSocketServer::
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
CNetSocketServer::
setFdHWM(int fd)
{
  fd_hwm_ = std::max(fd_hwm_, fd);
}

void
CNetSocketServer::
resetFdHWM(int fd)
{
  if (fd == fd_hwm_)
    --fd_hwm_;
}

/*TODO: Move to COS */
bool
CNetSocketServer::
makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *hostname)
{
  struct addrinfo hint;

  memset(&hint, 0, sizeof(hint));

  hint.ai_family   = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_flags    = AI_PASSIVE;

  CAutoFree<char> nodename = strdup(hostname);

  // <name>:<port>
  char *pc = strchr(nodename.get(), ':');

  if (! pc) {
    errno = EINVAL;
    return false;
  }

  *pc = '\0';

  const char *portStr = pc + 1;

  CAutoFreeAddrInfo info;

  if (getaddrinfo(nodename.get(), portStr, &hint, &info) != 0) {
    errno = EINVAL;
    return false;
  }

  memcpy(sa, info->ai_addr, info->ai_addrlen);

  *len = info->ai_addrlen;

  return true;
}

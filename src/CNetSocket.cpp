#include <CNetSocket.h>
#include <CFile.h>
#include <COSRead.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

CNetSocket::
CNetSocket(const std::string &name) :
 is_server_(false), fd_(-1), fd_client_(-1), name_(name), fd_hwm_(0)
{
}

CNetSocket::
~CNetSocket()
{
  if (fd_ != 0)
    close();
}

bool
CNetSocket::
openClient()
{
  is_server_ = false;

  return openClientServer();
}

bool
CNetSocket::
openServer()
{
  is_server_ = true;

  return openClientServer();
}

bool
CNetSocket::
openClientServer()
{
  struct sockaddr_storage sa;
  socklen_t               sa_len;

  if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len,
                       name_.c_str(), is_server_))
    return false;

  fd_ = socket(AF_INET, SOCK_STREAM, 0);

  if (fd_ < 0)
    return false;

  setFdHWM(fd_);

  int socket_option_value = 1;

  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                 &socket_option_value, sizeof(socket_option_value)) != 0)
    return false;

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
CNetSocket::
waitServer()
{
  if (! is_server_) {
    errno = ENOTSUP;
    return false;
  }

  fd_client_ = -1;

  while (true) {
    fd_set fd_set_read = fd_set_;

    if (select(fd_hwm_ + 1, &fd_set_read, NULL, NULL, NULL) < 0)
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
CNetSocket::
writeServer(const std::string &str)
{
  return writeServer(str.c_str(), str.size());
}

bool
CNetSocket::
writeServer(const char *data, size_t size)
{
  //TODO: use send
  return COSRead::writeall(fd_, data, size);
}

bool
CNetSocket::
writeClient(const std::string &str)
{
  return writeClient(str.c_str(), str.size());
}

bool
CNetSocket::
writeClient(const char *data, size_t size)
{
  if (fd_client_ < 0)
    return false;

  //TODO: use send
  return COSRead::writeall(fd_client_, data, size);
}

bool
CNetSocket::
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
CNetSocket::
readServer(char *data, size_t size, int *nread)
{
  //TODO: use recv
  *nread = int(::read(fd_, data, size));

  if (*nread < 0)
    return false;

  return true;
}

bool
CNetSocket::
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
CNetSocket::
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
CNetSocket::
close()
{
  if (is_server_) {
    for (int fd = 0; fd <= fd_hwm_; ++fd)
      if (FD_ISSET(fd, &fd_set_))
        ::close(fd);
  }
  else
    ::close(fd_);

  fd_ = -1;

  return true;
}

bool
CNetSocket::
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
CNetSocket::
setFdHWM(int fd)
{
  fd_hwm_ = std::max(fd_hwm_, fd);
}

void
CNetSocket::
resetFdHWM(int fd)
{
  if (fd == fd_hwm_)
    --fd_hwm_;
}

/*TODO: Move to COS */
/*TODO: Add auto_free class */
bool
CNetSocket::
makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *name, bool will_bind)
{
  struct addrinfo hint;

  memset(&hint, 0, sizeof(hint));

  hint.ai_family   = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  if (will_bind)
    hint.ai_flags = AI_PASSIVE;

  char *nodename = strdup(name);

  char *servicename = strchr(nodename, ':');

  if (servicename == NULL) {
    free(nodename);
    errno = EINVAL;
    return false;
  }

  *servicename = '\0';

  ++servicename;

  struct addrinfo *info = NULL;

  if (getaddrinfo(nodename, servicename, &hint, &info) != 0) {
    free(nodename);
    freeaddrinfo(info);
    errno = EINVAL;
    return false;
  }

  free(nodename);

  memcpy(sa, info->ai_addr, info->ai_addrlen);

  *len = info->ai_addrlen;

  freeaddrinfo(info);

  return true;
}

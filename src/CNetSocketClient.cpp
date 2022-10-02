#include <CNetSocketClient.h>
#include <CFile.h>
#include <COSRead.h>
#include <CAutoFree.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

CNetSocketClient::
CNetSocketClient(const std::string &hostname) :
 hostname_(hostname)
{
}

CNetSocketClient::
~CNetSocketClient()
{
  close();
}

bool
CNetSocketClient::
open()
{
  struct sockaddr_storage sa;
  socklen_t               sa_len;

  if (! makeSocketAddr(reinterpret_cast<struct sockaddr *>(&sa), &sa_len, hostname_.c_str()))
    return false;

  //----

  // Open socket

  int protocol = 0;

  fd_ = ::socket(AF_INET, SOCK_STREAM, protocol);

  if (fd_ < 0)
    return false;

  //----

  int socket_option_value = 1;

  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                 &socket_option_value, sizeof(socket_option_value)) != 0)
    return false;

  //----

  // Connect socket

  if (::connect(fd_, reinterpret_cast<struct sockaddr *>(&sa), sa_len) != 0)
    return false;

  return true;
}

bool
CNetSocketClient::
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
CNetSocketClient::
read(char *data, size_t size, int *nread)
{
  //TODO: use recv
  *nread = int(::read(fd_, data, size));

  if (*nread < 0)
    return false;

  return true;
}

bool
CNetSocketClient::
write(const std::string &str)
{
  return write(str.c_str(), str.size());
}

bool
CNetSocketClient::
write(const char *data, size_t size)
{
  //TODO: use send
  return COSRead::writeall(fd_, data, size);
}

bool
CNetSocketClient::
close()
{
  if (fd_ >= 0)
    ::close(fd_);

  fd_ = -1;

  return true;
}

/*TODO: Move to COS */
bool
CNetSocketClient::
makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *hostname)
{
  struct addrinfo hint;

  memset(&hint, 0, sizeof(hint));

  hint.ai_family   = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  CAutoFree<char> nodename = strdup(hostname);

  char *servicename = strchr(nodename.get(), ':');

  if (! servicename) {
    errno = EINVAL;
    return false;
  }

  *servicename = '\0';

  ++servicename; // service name points to port number

  CAutoFreeAddrInfo info;

  if (getaddrinfo(nodename.get(), servicename, &hint, &info) != 0) {
    errno = EINVAL;
    return false;
  }

  memcpy(sa, info->ai_addr, info->ai_addrlen);

  *len = info->ai_addrlen;

  return true;
}

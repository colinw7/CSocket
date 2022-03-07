#ifndef CNET_SOCKET_CLIENT_H
#define CNET_SOCKET_CLIENT_H

#include <string>
#include <sys/socket.h>

class CNetSocketClient {
 private:
  int         fd_;
  std::string hostname_;

 public:
  CNetSocketClient(const std::string &hostname);
 ~CNetSocketClient();

  bool open();

  bool read(std::string &str);
  bool read(char *data, size_t size, int *nread);

  bool write(const std::string &str);
  bool write(const char *data, size_t size);

  bool close();

 private:
  bool makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *hostname);
};

#endif

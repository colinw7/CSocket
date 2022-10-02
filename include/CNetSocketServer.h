#ifndef CNET_SOCKET_SERVER_H
#define CNET_SOCKET_SERVER_H

#include <string>
#include <sys/socket.h>

class CNetSocketServer {
 public:
  CNetSocketServer(const std::string &hostname);
 ~CNetSocketServer();

  bool open();

  bool wait();

  bool read(std::string &str);
  bool read(char *data, size_t size, int *nread);

  bool write(const std::string &str);
  bool write(const char *data, size_t size);

  bool close();
  bool closeClient();

 private:
  void setFdHWM(int fd);
  void resetFdHWM(int fd);

  bool makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *hostname);

 private:
  int         fd_        { -1 };
  int         fd_client_ { -1 };
  std::string hostname_;
  fd_set      fd_set_;
  int         fd_hwm_    { 0 };
};

#endif

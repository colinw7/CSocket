#ifndef CNET_SOCKET_H
#define CNET_SOCKET_H

#include <string>
#include <sys/socket.h>

/*!
 * Create socket for named host and/or port
 */
class CNetSocket {
 public:
  CNetSocket(const std::string &name);

  virtual ~CNetSocket();

  bool openClient();
  bool openServer();

  bool waitServer();

  bool writeServer(const std::string &str);
  bool writeServer(const char *data, size_t size);

  bool writeClient(const std::string &str);
  bool writeClient(const char *data, size_t size);

  bool readServer(std::string &str);
  bool readServer(char *data, size_t size, int *nread);

  bool readClient();
  bool readClient(std::string &str);
  bool readClient(char *data, size_t size, int *nread);

  bool close();
  bool closeClient();

 private:
  bool openClientServer();

  void setFdHWM(int fd);
  void resetFdHWM(int fd);

  static bool makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *name, bool abstract);

 private:
  bool        is_server_ { false };
  int         fd_ { -1 };
  int         fd_client_ { -1 };
  std::string name_;
  fd_set      fd_set_;
  int         fd_hwm_ { 0 };
};

#endif

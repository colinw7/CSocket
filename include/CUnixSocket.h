#ifndef CUNIX_SOCKET_H
#define CUNIX_SOCKET_H

#include <string>
#include <sys/socket.h>

class CUnixSocket {
 private:
  bool        is_server_;
  int         fd_;
  int         fd_client_;
  std::string name_;
  fd_set      fd_set_;
  int         fd_hwm_;
  bool        datagram_;
  bool        abstract_;

 public:
  CUnixSocket(const std::string &name);

  virtual ~CUnixSocket();

  void setDatagram(bool datagram) { datagram_ = datagram; }

  void setAbstract(bool abstract) { abstract_ = abstract; }

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

  virtual void handleRequest(const std::string &) { }

 private:
  bool openClientServer();

  void setFdHWM(int fd);
  void resetFdHWM(int fd);

  static bool makeSocketAddr(struct sockaddr *sa, socklen_t *len, const char *name, bool abstract);
};

#endif

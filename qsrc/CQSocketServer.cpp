#include <CQSocketServer.h>

#include <CSocket.h>
#include <COSUser.h>

#include <QApplication>
#include <QSocketNotifier>

#include <iostream>

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  auto *server = new CQSocketServer;
  assert(server);

  return app.exec();
}

CQSocketServer::
CQSocketServer()
{
  int port = CSocket::getFreePort();

  std::string hostname = "//" + COSUser::getHostName() + ":" + std::to_string(port);
  std::cerr << hostname << "\n";

  socket_ = new CSocket(hostname);

  if (! socket_->openServer()) {
    std::cerr << "Failed to open server\n";
    exit(1);
  }

  sn_ = new QSocketNotifier(socket_->fd(), QSocketNotifier::Read, this);

  sn_->setEnabled(true);

  connect(sn_, SIGNAL(activated(int)), this, SLOT(readyRead()));
}

void
CQSocketServer::
readyRead()
{
  if (! socket_->waitServer())
    return;

  std::string msg;

  if (! socket_->readClient(msg))
    return;

  std::cerr << msg << "\n";

  socket_->closeClient();
}

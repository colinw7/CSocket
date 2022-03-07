#include <CSocket.h>
#include <CStrUtil.h>

static bool handle_request(CSocket *socket, const std::string &msg);
static bool send_header(CSocket *socket, const std::string &msg);

int
main(int argc, char **argv)
{
  CSocket socket("unix");

  socket.openServer();

  while (true) {
    if (! socket.waitServer())
      break;

    std::string msg;

    if (! socket.readClient(msg))
      continue;

    if (msg.size() != 0)
      handle_request(&socket, msg);

    socket.closeClient();
  }

  socket.close();

  exit(0);
}

static bool
handle_request(CSocket *socket, const std::string &msg)
{
  send_header(socket, msg + ": ok");

  return true;
}

static bool
send_header(CSocket *socket, const std::string &msg)
{
  if (! socket->writeClient(msg))
    return false;

  return true;
}

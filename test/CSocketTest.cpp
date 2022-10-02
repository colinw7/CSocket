#include <CSocket.h>
#include <iostream>

int
main(int argc, char **argv)
{
  int port = 0;

  for (int i = 1; i < argc; ++i)
    port = std::atoi(argv[i]);

  CSocket socket("unix");

  int freePort = socket.getFreePort(port);

  std::cerr << "Free port " << freePort << "\n";

  exit(0);
}

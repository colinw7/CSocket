#include <CSocket.h>
#include <CReadLine.h>
#include <CStrUtil.h>

#include <iostream>

static bool send_command(const std::string &url);

int
main(int argc, char **argv)
{
  CReadLine readline;

  std::string line;

  for (;;) {
    line = readline.readLine();

    std::vector<std::string> words;

    CStrUtil::addWords(line, words);

    int num_words = words.size();
    if (num_words == 0) continue;

    if (CStrUtil::casecmp(words[0], "exit") == 0)
      exit(0);

    send_command(line);
  }
}

static bool
send_command(const std::string &msg)
{
  CSocket socket("unix");

  if (! socket.openClient())
    return false;

  if (! socket.writeServer(msg))
    return false;

  while (true) {
    std::string msg;

    if (! socket.readServer(msg))
      return false;

    if (msg.size() == 0) {
      std::cout << "EOF\n";
      break;
    }

    std::cout << msg << "\n";
  }

  socket.close();

  return true;
}

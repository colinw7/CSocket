#include <CSocket.h>
#include <CReadLine.h>
#include <CStrUtil.h>
#include <COSUser.h>

#include <iostream>

static bool getUrl(const std::string &url);

static int port = 8000;

int
main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i)
    port = std::atoi(argv[i]);

  //---

  CReadLine readline;

  std::string line;

  for (;;) {
    line = readline.readLine();

    std::vector<std::string> words;

    CStrUtil::addWords(line, words);

    int num_words = words.size();

    if (num_words == 0)
      continue;

    if      (CStrUtil::casecmp(words[0], "exit") == 0)
      exit(0);
    else if (CStrUtil::casecmp(words[0], "get" ) == 0) {
      if (num_words < 2) {
        std::cerr << "Missing url\n";
        continue;
      }

      getUrl(words[1]);
    }
    else {
      std::cerr << "Illegal command " << words[0] << "\n";
      std::cerr << " Commands are get <url>, exit\n";
    }
  }
}

static bool
getUrl(const std::string &url_file)
{
  auto url_file1 = url_file;

  auto pos = url_file1.find('/');

  if (pos == std::string::npos) {
    auto hostname = COSUser::getHostName();

    url_file1 = hostname + "/" + url_file;

    pos = url_file1.find('/');
  }

  std::string url  = url_file1.substr(0, pos);
  std::string file = url_file1.substr(pos + 1);

  url = "//" + url + ":" + std::to_string(port);

  std::cerr << url << "\n";

  CSocket socket(url);

  if (! socket.openClient())
    return false;

  std::string http_msg = "GET /" + file + " HTTP/1.0\r\n\r\n";

  if (! socket.writeServer(http_msg))
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

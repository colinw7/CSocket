#include <CSocket.h>
#include <CFile.h>
#include <CFileUtil.h>
#include <CStrUtil.h>
#include <COSUser.h>

#include <cstring>
#include <iostream>

#define HEADER\
  "HTTP/1.0 %s\r\n"\
  "Server: AUP-ws\r\n"\
  "Content-Length: %ld\r\n"

#define CONTENT_TEXT\
  "Content-Type: text/html\r\n\r\n"

#define CONTENT_JPEG\
  "Content-Type: image/jpeg\r\n\r\n"

#define HTML_NOTFOUND\
  "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"\
  "<html><head><title>Error 404</title>\n"\
  "</head><body>\n"\
  "<h2>AUP-ws server can't find document</h2>"\
  "</body></html>\r\n"

#define DEFAULT_DOC "index.html"
#define WEB_ROOT    "/home/colinw/dev/progs/ipc/test/CSocket"

static bool handle_request(CSocket *socket, const std::string &msg);
static bool send_header(CSocket *socket, const std::string &msg,
                        unsigned int data_len);
static bool send_header(CSocket *socket, const std::string &msg,
                        unsigned int data_len, const std::string &path);
static bool send_content(CSocket *socket, const std::string &path);

int
main(int argc, char **argv)
{
  int port = 0;

  for (int i = 1; i < argc; ++i)
    port = std::atoi(argv[i]);

  int freePort = CSocket::getFreePort(port);

  auto hostname = "//" + COSUser::getHostName() + ":" + std::to_string(freePort);

  std::cerr << hostname << "\n";

  CSocket socket(hostname);

  if (! socket.openServer()) {
    std::cerr << "Failed to open server\n";
    exit(1);
  }

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
  std::vector<std::string> words;

  CStrUtil::addWords(msg, words);

  int num_words = words.size();

  if (num_words < 2 || CStrUtil::casecmp(words[0], "get") != 0) {
    std::cerr << "Unknown request\n";
    return false;
  }

  std::string path = WEB_ROOT + words[1];

  if (CFile::isDirectory(path)) {
    if (path[path.size() - 1] != '/')
      path += '/';

    path += DEFAULT_DOC;
  }

  if (! CFile::exists(path)) {
    send_header(socket, "404 Not Found", strlen(HTML_NOTFOUND));

    socket->writeClient(HTML_NOTFOUND);

    return false;
  }

  CFile file(path);

  send_header (socket, "200 OK", file.getSize());
  send_content(socket, path);

  size_t        num_read;
  unsigned char buffer[512];

  file.read(buffer, sizeof(buffer), &num_read);

  while (num_read > 0) {
    socket->writeClient((char *) buffer, num_read);

    file.read(buffer, sizeof(buffer), &num_read);
  }

  file.close();

  return true;
}

static bool
send_header(CSocket *socket, const std::string &msg, unsigned int data_len)
{
  return send_header(socket, msg, data_len, "");
}

static bool
send_header(CSocket *socket, const std::string &msg, unsigned int data_len,
            const std::string &path)
{
  std::string msg1 = CStrUtil::strprintf(HEADER, msg.c_str(), (long) data_len);

  if (! socket->writeClient(msg1))
    return false;

  return true;
}

static bool
send_content(CSocket *socket, const std::string &path)
{
  CFileType data_type = CFileUtil::getType(path);

  if (data_type == CFILE_TYPE_IMAGE) {
    CFileType image_type = CFileUtil::getImageType(path);

    if (image_type == CFILE_TYPE_IMAGE_JPG)
      socket->writeClient(CONTENT_JPEG);
    else
      return false;
  }
  else
    socket->writeClient(CONTENT_TEXT);

  return true;
}

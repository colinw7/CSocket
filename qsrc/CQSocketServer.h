#include <QWidget>

class CSocket;
class QSocketNotifier;

class CQSocketServer : public QObject {
  Q_OBJECT

 public:
  CQSocketServer();

 private slots:
  void readyRead();

 private:
  CSocket*         socket_ { nullptr };
  QSocketNotifier* sn_     { nullptr };
};

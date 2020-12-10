#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QLineEdit>
#include <QGridLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QMap>

#include "chatdialog.h"

const char CMD_BEGIN = 0x02;
const char CMD_END = 0x03;
const char CMD_LOGIN = 0x11;
const char CMD_LIST = 0x12;
const char CMD_CHAT = 0x13;
const int IDLEN = 4;
const int MAX_NO_HEART_TIMES = 5;


class ChatDialog;
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

public slots:
    void login();
    void connected();
    void disconnected();
    void recv();
    void sendList();
    void sendChatData(QString otherID, QByteArray data);
    void showChatDialog(QModelIndex index);

public:
    ChatDialog* findChatDialog(const QString& ID);

private:
    void sendLogin();
    void analyze(const QByteArray& buf);

private:
    QTcpSocket* _socket;
    QListWidget* _list;
    QLineEdit* _ipEdit;
    QLineEdit* _portEdit;
    QPushButton* _pb;
    QByteArray _recvBuf;
    QString _userID;
    QLabel* _IDLabel;
    QTimer* _timer;
    QMap <QString, ChatDialog*> _chatDialogMap;
    int _noheartTimes;
};

#endif // WIDGET_H

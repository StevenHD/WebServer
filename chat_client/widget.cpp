#include "widget.h"
#include "chatdialog.h"

Widget::Widget(QWidget *parent) : QWidget(parent), _noheartTimes(0)
{
    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("Online User List:"), this), 0, 0, 1, 1);

    _list = new QListWidget(this);
    connect(_list, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showChatDialog(QModelIndex)));
    layout->addWidget(_list, 1, 0, 1, 2);

    _IDLabel = new QLabel("UserID: ", this);
    layout->addWidget(_IDLabel, 2, 0, 1, 1);

    layout->addWidget(new QLabel(tr("Server IP:"), this), 3, 0, 1, 1);
    _ipEdit = new QLineEdit("172.20.10.2", this);
    layout->addWidget(_ipEdit, 3, 1, 1, 1);

    layout->addWidget(new QLabel(tr("Server Port:"), this), 4, 0, 1, 1);
    _portEdit = new QLineEdit("8888", this);
    layout->addWidget(_portEdit, 4, 1, 1, 1);

    _pb = new QPushButton(tr("LOGIN"), this);
    layout->addWidget(_pb, 5, 0, 1, 2);
    connect(_pb, SIGNAL(clicked()), this, SLOT(login()));

    _socket = new QTcpSocket(this);
    connect(_socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(_socket, SIGNAL(readyRead()), this, SLOT(recv()));
    connect(_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));

    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(sendList()));
}

/* 接收服务器应答 */
void Widget::recv()
{
    _recvBuf.append(_socket->readAll());

    qDebug() << "recvBuf: " << _recvBuf.toHex() << endl;

    int begin, end;

    while (1)
    {
        begin = _recvBuf.indexOf(CMD_BEGIN);
        if (begin == -1)
        {
            _recvBuf.clear();
            break;
        }
        else
        {
            end = _recvBuf.indexOf(CMD_END, begin);
            if (end > begin)
            {
                analyze(_recvBuf.mid(begin, end - begin + 1));
                _recvBuf = _recvBuf.mid(end + 1);
            }
            else break;
        }
    }
}

void Widget::showChatDialog(QModelIndex index)
{
    qDebug() << __FUNCTION__ << endl;
    ChatDialog* chatDialog = findChatDialog(index.data().toString());
    chatDialog->show();
}

ChatDialog* Widget::findChatDialog(const QString& ID)
{
    ChatDialog* chatDialog;
    chatDialog = _chatDialogMap.value(ID, NULL);
    if (chatDialog == NULL)
    {
        chatDialog = new ChatDialog(ID, this);
        connect(chatDialog, SIGNAL(chatData(QString, QByteArray)),
                    this, SLOT(sendChatData(QString, QByteArray)));

        _chatDialogMap.insert(ID, chatDialog);
    }
    return chatDialog;
}

/* 分析数据包 */
void Widget::analyze(const QByteArray& buf)
{
    if (buf.at(1) == CMD_LOGIN)
    {
        _userID = buf.mid(1 + 1, IDLEN);
        _IDLabel->setText("UserID: " + _userID);
        _timer->start(1000);
    }
    else if (buf.at(1) == CMD_LIST)
    {
        int cnt = (buf.length() - 1 - 1 - 1) / IDLEN;

        int index = 2;
        _list->clear();

        /* 更新在线用户列表 */
        for (int i = 0; i < cnt; i ++ )
        {
            _list->addItem(buf.mid(index, IDLEN));
            index += IDLEN;
        }
        _noheartTimes = 0;
    }
    else if (buf.at(1) == CMD_CHAT)
    {
        int index = 2 + IDLEN;
        QString otherID = buf.mid(index, IDLEN);
        index += IDLEN;
        QByteArray chatData = buf.mid(index, buf.length() - index - 1);

        ChatDialog* chatDialog = findChatDialog(otherID);
        chatDialog->setChatList(chatData);
        chatDialog->show();
    }
}

/* 连接成功 */
void Widget::connected()
{
    qDebug() << __FUNCTION__ << endl;
    _pb->setEnabled(false);
    _ipEdit->setEnabled(false);
    _portEdit->setEnabled(false);
}

/* 断开连接 */
void Widget::disconnected()
{
    _list->clear();
    _pb->setEnabled(true);
    _ipEdit->setEnabled(true);
    _portEdit->setEnabled(true);
    _timer->stop();
}


/* 登录远端服务器 */
void Widget::login()
{
    if (_ipEdit->text().isEmpty() || _portEdit->text().isEmpty())
    {
        QMessageBox::information(this, "login", tr("IP and port should not be empty"));
        return;
    }
    _socket->connectToHost(_ipEdit->text(), _portEdit->text().toInt());
    sendLogin();
}

/* 发送登录数据包 */
void Widget::sendLogin()
{
    QByteArray buf;
    buf.append(CMD_BEGIN);
    buf.append(CMD_LOGIN);
    buf.append(CMD_END);
    _socket->write(buf);
}


/* 获取在线用户列表 */
void Widget::sendList()
{
    QByteArray buf;
    buf.append(CMD_BEGIN);
    buf.append(CMD_LIST);
    buf.append(CMD_END);
    _socket->write(buf);
    if (++ _noheartTimes >= MAX_NO_HEART_TIMES)
    {
        _socket->disconnectFromHost();
        disconnected();
    }
}

/* 发送聊天数据 */
void Widget::sendChatData(QString otherID, QByteArray data)
{
    QByteArray buf;
    buf.append(CMD_BEGIN);
    buf.append(CMD_CHAT);

    buf.append(otherID);
    buf.append(_userID);
    buf.append(data);

    buf.append(CMD_END);
    _socket->write(buf);
}

Widget::~Widget()
{
    _socket->disconnectFromHost();
}

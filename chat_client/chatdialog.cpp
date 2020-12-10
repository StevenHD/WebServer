#include "chatdialog.h"

ChatDialog::ChatDialog(QString otherID, QWidget* parent) : QDialog(parent), _otherID(otherID)
{
    QGridLayout* layout = new QGridLayout(this);
    _list = new QListWidget(this);
    layout->addWidget(_list, 0, 0, 1, 1);

    _edit = new QTextEdit(this);
    layout->addWidget(_edit, 1, 0, 1, 1);

    QPushButton* pb = new QPushButton("send", this);
    connect(pb, SIGNAL(clicked()), this, SLOT(sendChatData()));
    layout->addWidget(pb, 2, 0, 1, 1);

    this->setWindowTitle("You are chatting with " + _otherID);
}

void ChatDialog::sendChatData()
{
    QByteArray data = _edit->toPlainText().toUtf8();
    emit chatData(_otherID, _edit->toPlainText().toUtf8());
    _edit->clear();
    _list->addItem("me: " + data);
}

void ChatDialog::setChatList(const QByteArray &data)
{
    _list->addItem(_otherID + ": " + data);
}

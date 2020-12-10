#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QGridLayout>
#include <QPushButton>

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    ChatDialog(QString otherID, QWidget* parent = NULL);

signals:
    void chatData(QString otherID, QByteArray data);

public slots:
    void sendChatData();

public:
    void setChatList(const QByteArray &data);

private:
    QListWidget* _list;
    QTextEdit* _edit;
    QString _otherID;
};

#endif // CHATDIALOG_H

#pragma once

#include <QThread>
#include <QObject>
#include <QDebug>
#include <QMutex>

class QTcpSocket;
class Ui;
class Server;

class Session : public QThread
{
    Q_OBJECT
public:
    Session(qintptr socketDescr, Ui *ui, QObject *parent = Q_NULLPTR);
    virtual ~Session();

    QString getAccount() const;
    void run(void);

    Server *server() const;

    void setAccount(const QString &newAccount);

    QTextStream &output();

public slots:
    void request_exit();
    void notifyUser(const QString &msg);

    void flush();

signals:

private slots:   
    void close();

private:
    const qintptr socketDescr;
    QTcpSocket *socket;
    QTextStream _output;
    QString os;

    QMutex handleMutex;

    Ui *ui;

    QString account;
};

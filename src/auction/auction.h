#pragma once

#include <QThread>
#include <QObject>
#include <QDebug>

#include "mysocket.h"

class Socket;
class Handler;

class Session : public QThread
{
    Q_OBJECT
public:
    Session(int socketDescr, QObject *parent = Q_NULLPTR);
    ~serverThread();

private:
    void run(void);

public slots:
    void sendDataSlot(int sockDesc, const QByteArray &data);

signals:
    void dataReady(const QString &ip, const QByteArray &data);
    void sendData(int sockDesc, const QByteArray &data);
    void disconnectTCP(int sockDesc);

private slots:   
    void recvDataSlot(const QString &ip, const QByteArray &data);
    void disconnectToHost(void);

private:
    QTcpSocket *socket;

    int socketDescr;
};

#endif // SERVERTHREAD_H

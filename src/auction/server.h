#pragma once

#include "session.h"

#include <QThreadPool>
#include <QTcpServer>
#include <QSet>
#include <QHash>
#include <QTimer>

#include <QMutex>

#include <queue>

class Ui;
class Session;

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(Ui *ui, QObject *parent = Q_NULLPTR);

    bool isLoggedIn(const QString &userName) const;

public slots:
    void login(Session *session);
    void logout(Session *session);
    void on_saleStart(qint64 timestamp, const QString &item);
    void notifyUser(const QString &user, const QString &msg);

    void sale_conclude();

protected:
    virtual void incomingConnection(qintptr socketDescr);
    virtual void timerEvent(QTimerEvent *event);

private:
    Ui *ui;
    QSet<Session*> anonSessions;
    QHash<QString, Session*> accountsSessions;

    qint64 lastSaleConclude;
    QMutex sale_conclude_mutex;

//    QSet<int> saleTimers;
//    std::priority_queue<> saleTimers;
//    QTimer *sales_timer;
//    QThreadPool *threadPool;
};

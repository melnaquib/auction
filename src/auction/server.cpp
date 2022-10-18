#include "config.h"
#include "server.h"
#include "session.h"
#include "db.h"

#include <QTimer>
#include <QTimerEvent>
#include <QDateTime>
#include <QSqlDatabase>
#include <QtConcurrent/QtConcurrent>

Server::Server(Ui *ui, QObject *parent) :
    QTcpServer(parent), ui(ui) {

//    connect(sales_timer, &QTimer::timeout, this, &Server::sale_conclude, Qt::QueuedConnection);
//    sales_timer->setSingleShot(true);
//    sales_timer->start(0);

    sale_conclude();
}

void Server::incomingConnection(qintptr socketDescr) {
    auto session = new Session(socketDescr, ui, this);
    anonSessions << session;
    session->start();

    QTcpServer::incomingConnection(socketDescr);
}

bool Server::isLoggedIn(const QString &userName) const {
    return accountsSessions.contains(userName);
}

void Server::login(Session *session) {
    anonSessions.remove(session);
    if(accountsSessions.contains(session->getAccount())) {
        accountsSessions[session->getAccount()]->request_exit();
    }
    accountsSessions[session->getAccount()] = session;
}

void Server::logout(Session *session) {
    if(session) {
        if(anonSessions.contains(session)) {
            anonSessions.remove(session);
        }
        if(accountsSessions.contains(session->getAccount())
                && session == accountsSessions[session->getAccount()]) {
            accountsSessions.remove(session->getAccount());
        }
    }
}

void Server::notifyUser(const QString &user, const QString &msg) {
    bool online = accountsSessions.contains(user);
    qInfo() << "NOTTIFY : " << user << (online ? " : ONLINE : " : " : OFFLINE : ") << msg;
    if(online) {
        Session *session = accountsSessions[user];
//        QMetaObject::invokeMethod(session, SLOT(notifyUser(QString)), Qt::QueuedConnection, msg);
        QMetaObject::invokeMethod(
                    session, SLOT(notifyUser(QString)),
                    Qt::QueuedConnection,
                    Q_ARG(QString, msg));

    }
}

void Server::on_saleStart(qint64 timestamp, const QString &item) {
//    qint64 auction_interval = QDateTime::currentDateTime().toMSecsSinceEpoch() - timestamp + SALE_TIMEOUT + SALE_TIMEOUT_MARGIN;

//    if(auction_interval < sales_timer->remainingTime()) {
//        sales_timer->setInterval(auction_interval);
//    }
//    int interval = 10;
//    saleTimers << startTimer(interval, Qt::VeryCoarseTimer);

}

void Server::timerEvent(QTimerEvent *event) {
    if(saleTimers.contains(event->timerId())) {
        sale_conclude();
        saleTimers.remove(event->timerId());
    }
}

void Server::sale_conclude() {
    QHash<QString, QString> sold = db::sale_conclude();
    for(auto i : sold) {
        notifyUser(i[0], i[1]);
    }
}

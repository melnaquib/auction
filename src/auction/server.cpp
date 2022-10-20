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
    QTcpServer(parent), ui(ui), lastSaleConclude(0) {

//    connect(sales_timer, &QTimer::timeout, this, &Server::sale_conclude, Qt::QueuedConnection);
//    sales_timer->setSingleShot(true);
//    sales_timer->start(0);

    db::connect();
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
        QMetaObject::invokeMethod(
                    session, "notifyUser",
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
    startTimer(SALE_TIMEOUT + SALE_TIMEOUT_MARGIN, Qt::VeryCoarseTimer);

}

void Server::timerEvent(QTimerEvent *event) {
    //TODO filter
    //if timerevent in saletimers
    sale_conclude();
}

void Server::sale_conclude() {
    QMutexLocker mlocker(&sale_conclude_mutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if( (now - lastSaleConclude) > SALE_TIMEOUT_INTERLUDE_MIN) {
        lastSaleConclude = now;
        QHash<QString, QString> sold = db::sale_conclude();
        for(auto account : sold.keys()) {
            notifyUser(account, sold[account]);
        }
    }

}

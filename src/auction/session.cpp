#include "session.h"
#include "server.h"
#include "db.h"
#include "ui.h"
#include "config.h"

#include <QDateTime>
#include <QTcpSocket>

Session::Session(qintptr socketDescr, Ui *ui, QObject *parent) :
    QThread(parent),
    socketDescr(socketDescr), socket(Q_NULLPTR), ui(ui), os(QString()), _output(&os, QIODevice::ReadWrite) {
}

Session::~Session() {
}

void Session::notifyUser(const QString &msg) {
    output() << NEWL "NOTIFY" NEWL << msg << NEWL;
    flush();
}

void Session::close() {
    server()->logout(this);
    if(socket) {
        if(socket->isOpen()) {
            socket->close();
        }
        socket->deleteLater();
        socket = Q_NULLPTR;
    }
}

void Session::request_exit() {
    requestInterruption();
}

QString Session::getAccount() const {
    return account;
}

void Session::setAccount(const QString &aaccount) {
    account = aaccount;
}

QTextStream &Session::output() {
    return _output;
}


void Session::flush() {
    auto ba = _output.readAll().toLatin1();
    socket->write(ba);
    socket->flush();
}

Server *Session::server() const {
    return qobject_cast<Server*>(parent());
}

void Session::run(void)
{

    socket = new QTcpSocket();
    //    _output.setDevice(socket);

    if (!socket->setSocketDescriptor(socketDescr)) {
        qWarning() << "Connection dropped early";
        deleteLater();
        return ;
    }

    if(!db::connect()) {
        deleteLater();
        return;
    }

    quint64 counter = 0;
    ui->start(this, QDateTime::currentMSecsSinceEpoch(), counter);
    flush();
    while(!isInterruptionRequested()) {
        socket->waitForReadyRead(5000);
        auto buffer = socket->readLine();
        if(!buffer.isEmpty()) {
            QMutexLocker l(&handleMutex);
            ui->handle(this, QDateTime::currentMSecsSinceEpoch(), counter++, QLatin1StringView(buffer));
        }
    }

    db::close();
    close();
    deleteLater();
}

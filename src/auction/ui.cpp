#include "ui.h"
#include "session.h"
#include "server.h"
#include "db.h"

#include <QDebug>
#include <QDateTime>

#define NEWL "\r\n"

Ui::Ui(QObject *parent) :
    QObject(parent) {

    handlers["list"] = {&Ui::list, 2};
    handlers["exit"] = {&Ui::exit, 1};
    handlers["fund"] = {&Ui::fund, 3};
    handlers["item"] = {&Ui::item, 3};
    handlers["sale"] = {&Ui::sale, 3};
    handlers["buy"] = {&Ui::buy, 2};
    handlers["auction"] = {&Ui::auction, 3};
    handlers["bid"] = {&Ui::bid, 3};
}

bool Ui::handle_login(Session *session, qint64 timestamp,  quint64 counter, QLatin1StringView buffer) {
    QString username = buffer.trimmed().toString();
    if(username.isEmpty())
        return false;

    username.detach();
    if(!db::account_add(username)) {
        session->output() << "Couldn't add account : " << username;
        return false;
    }

    session->setAccount(username);
    session->server()->login(session);
    return true;
}

bool Ui::handle(Session *session, qint64 timestamp,  quint64 counter, QLatin1StringView buffer) {
    if(session->getAccount().isEmpty()) {
        bool ok = handle_login(session, timestamp, counter, buffer);
        if(ok) prompt(session, timestamp, counter);
        else prompt_login(session, timestamp, counter);
        session->flush();
        return ok;
    }

    QStringTokenizer tokenizer = buffer.tokenize(u' ');
    QList<QLatin1StringView> cmd;
    for(QLatin1StringView s : tokenizer) {
        s = s.trimmed();
        if(!s.isEmpty())
            cmd << s;
    }

    qInfo() << "HANDLE " << counter << cmd;
    header(session, timestamp, counter, cmd);

    bool ok = true;
    if(cmd.isEmpty()) {
        qWarning() << "CMD Empty!";
        ok = false;
    } else if(!handlers.contains(cmd[0])) {
        qWarning() << "CMD Not Found!";
        ok = false;
    }
    if(ok) {
        auto spec = handlers[cmd[0]];
        Handler handler = spec.first;
        int argCount = spec.second;
        if(cmd.size() != argCount) {
            qWarning() << "CMD Invalid Syntax!";
            ok = false;
        } else {
            ok = (this->*handler)(session, timestamp, counter, cmd);
        }
    }
    footer(session, timestamp, counter, cmd, ok);
    prompt(session, timestamp, counter);
    session->flush();
    return ok;
}

bool Ui::start(Session *session, qint64 timestamp,  quint64 counter) {
    prompt_login(session, timestamp, counter);
    session->flush();
    return true;
}

void Ui::prompt(Session *session, qint64 timestamp,  quint64 counter) {
    session->output() << NEWL " AUCTION : " << counter << " : ";
}

void Ui::prompt_login(Session *session, qint64 timestamp,  quint64 counter) {
    session->output() << NEWL " USER NAME ? : " << counter << " : ";
}

void Ui::header(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
}

void Ui::footer(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd, bool ok) {
    session->output() << NEWL "NO. :" << counter << (ok ? " TRUE " : " FALSE ") << NEWL;
}

bool Ui::help(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    return true;
}

void Ui::showList(Session *session, const QStringList &list) {
    session->output() << NEWL "\t" << list.join(NEWL "\t") << NEWL;
}

bool Ui::list(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    if("me" == cmd[1]) {
        QString account = session->getAccount();

        session->output() << "ALL ITEMS : " NEWL;
        showList(session, db::item_list(account));
        session->output() << "SALES : " NEWL;
        showList(session, db::sale_list(account));
        session->output() << "AUCTIONS : " NEWL;
        showList(session, db::auction_list(account));
        session->output() << "FUND : " << db::fund(account);
    } else {
        QStringList res;
        if("users" == cmd[1]) {
            res = db::account_list();
        } else if("sales" == cmd[1]) {
            res = db::sale_list();
        } else if("auctions" == cmd[1]) {
            res = db::auction_list();
        }
        showList(session, res);
    }
    return true;
}

bool Ui::exit(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    session->output() << NEWL "Disconnecting" NEWL;
    session->request_exit();
    return true;
}

bool Ui::fund(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    bool add = "-" != cmd[1];
    qint64 amount = QString(cmd[2]).toInt();
    amount *= (add ? 1 : -1);
    return db::fund_add(session->getAccount(), amount);
}

bool Ui::item(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    QString account = session->getAccount();
    bool add = "-" != cmd[1];
    QLatin1StringView item = cmd[2];

    return add ? db::item_add(account, item) : db::item_del(account, item);
}

bool Ui::sale(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    auto account = session->getAccount();
    QLatin1StringView item = cmd[1];
    qint64 amount = QString(cmd[2]).toInt(nullptr, 10);
    bool ok = db::sale_add(timestamp, account, item, amount);
    if(ok)
        ok = QMetaObject::invokeMethod(
                    session->server(), "on_saleStart",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, timestamp), Q_ARG(QString, item.toString()));
    return ok;
}

bool Ui::buy(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    auto account = session->getAccount();
    QLatin1StringView item = cmd[1];
    QPair<QString, QString> res = db::sale_buy(timestamp, account, item);
    bool ok = !res.first.isEmpty();
    if(ok) {
        QMetaObject::invokeMethod(
                    session->server(), "notifyUser",
                    Qt::QueuedConnection,
                    Q_ARG(QString, res.first), Q_ARG(QString, res.second));
    }
    return ok;
}

bool Ui::auction(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    auto account = session->getAccount();
    QLatin1StringView item = cmd[1];
    qint64 amount = QString(cmd[2]).toInt();
    return db::auction_add(timestamp, account, item, amount);
}

bool Ui::bid(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd) {
    auto account = session->getAccount();
    QLatin1StringView item = cmd[1];
    qint64 amount = QString(cmd[2]).toInt();
    bool ok = db::auction_bid(timestamp, account, item, amount);
    if(ok)
        QMetaObject::invokeMethod(
                    session->server(), "on_saleStart",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, timestamp), Q_ARG(QString, item.toString()));
    return ok;
}

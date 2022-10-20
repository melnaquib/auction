#pragma once

#include <QThread>
#include <QObject>
#include <QLatin1StringView>
#include <QHash>


class Session;
class Server;
class Ui;

using Handler = bool (Ui::*)(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

//inline std::intptr_t qHash(const Handler &handler) noexcept {
//    return reinterpret_cast<std::intptr_t>(const_cast<Handler &>(handler))a ^ 0xFFFFFFFFFFFFFFFF;
//}

class Ui : public QObject
{
    Q_OBJECT
public:
    Ui(QObject *parent = nullptr);

public:
    bool start(Session *session, qint64 timestamp,  quint64 counter);
    bool handle(Session *session, qint64 timestamp,  quint64 counter, QLatin1StringView buffer);
    bool handle_login(Session *session, qint64 timestamp,  quint64 counter, QLatin1StringView buffer);

    bool help(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);
    bool handleDefault(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

    bool exit(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

    bool list(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

    bool fund(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);
    bool item(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

    bool sale(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);
    bool buy(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

    bool auction(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);
    bool bid(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);

private:
    QHash<QString, QPair<Handler, int> > handlers;

    void prompt(Session *session, qint64 timestamp,  quint64 counter);
    void prompt_login(Session *session, qint64 timestamp,  quint64 counter);
    void header(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd);
    void footer(Session *session, qint64 timestamp,  quint64 counter, QList<QLatin1StringView> cmd, bool ok);
    void showList(Session *session, const QStringList &list);

};

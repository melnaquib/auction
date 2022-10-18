#pragma once

#include <QSqlDatabase>
#include <QThreadStorage>

class QThread;

namespace db {

    const qsizetype USER_NAME_LEN_MAX = 64;
    const qsizetype ITEM_NAME_LEN_MAX = 64;

    QString currentThreadConnName();
    QSqlDatabase db(const QString &connName = currentThreadConnName());

    bool init();
    bool connect(const QString &connName = currentThreadConnName());
    bool build(const QString &connName = currentThreadConnName());
    void close(const QString &connName = currentThreadConnName());

    QStringList account_list();
    bool account_add(const QString &account);

    qint64 fund(const QString &account);
    bool fund_add(const QString &account, qint64 amount);

    QStringList item_list(const QString &account = QString());
    bool item_add(const QString &account, QLatin1StringView item);
    bool item_del(const QString &account, QLatin1StringView item);

    QStringList sale_list(const QString &account = QString());
    bool sale_add(qint64 timestamp, const QString &account, QLatin1StringView item, qint64 price);
    bool sale_buy(qint64 timestamp, const QString &account, QLatin1StringView item);

    QStringList auction_list(const QString &account = QString());
    bool auction_add(qint64 timestamp, const QString &account, QLatin1StringView item, qint64 bid);
    bool auction_bid(qint64 timestamp, const QString &account, QLatin1StringView item, qint64 bid);

    QHash<QString, QString> sale_conclude();
}

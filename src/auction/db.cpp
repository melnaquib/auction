#include "config.h"
#include "db.h"
#include "db_res.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QFileInfo>
#include <QThread>
#include <QThreadStorage>

#define NEWL "\r\n"

typedef QLatin1StringView QLatin1String;

namespace db {

#define DB_NAME_DEFAULT "auction.sqlite3";

#define DB_FILTER_AUCTION " price != 0 AND buyer IS NOT NULL "
#define DB_FILTER_SALE " price != 0 AND buyer IS NULL "
#define DB_FILTER_SALE_AUCTION " price != 0 "
#define DB_FILTER_NOT_SALE " price = 0 AND buyer IS NULL "
#define DB_FILTER_NOT_AUCTION " price = 0 AND buyer IS NOT NULL "
#define DB_FILTER_NOT_AUCTION_NOT_SALE " price IS NULL "

QString currentThreadConnName() {
    return "__DB_CONN__" + QString::number(reinterpret_cast<qulonglong>(QThread::currentThread()));
}

QSqlDatabase db(const QString &connName) {
    return QSqlDatabase::database(connName);
}


bool connect(const QString &connectionName) {
    QString name = DB_NAME_DEFAULT;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(name);
    return db.open();
}

bool build(const QString &connName) {
    auto database = db(connName);
    QSqlQuery query(database);
    for(auto& stmt: DDL) {
        if(!query.exec(stmt)) {
            qWarning() << stmt;
            qWarning() << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool init() {
    QString connName = QSqlDatabase::defaultConnection;
    if(!connect(connName)) {
        qFatal("Couldn't open db!");
        return false;
    }
    qInfo() << "DB openned : " << QFileInfo(QSqlDatabase::database().databaseName()).absoluteFilePath();

    if(!build(connName)) {
        qFatal("Couldn't construct db!");
        return false;
    }
    qInfo() << "DB init.";
    return true;
}

void close(const QString &connName) {
    db(connName).close();
    QSqlDatabase::removeDatabase(connName);
}

QVariantHash fromRecord(QSqlRecord &record) {
    QVariantHash res;
    for(int i = 0; i < record.count(); ++i)
        res[record.fieldName(i)] = record.value(i);
    return res;
}

QStringList resultToList(QSqlQuery &query) {
    QStringList res;
    while(query.next()) {
        int cols = query.record().count();
        QString row = query.value(0).toString();
        for(int i = 1; i < cols; ++i)
            row += ", " + query.value(i).toString();
        res << row;
    }
    return res;
}

qint64 sale_fee() {
    return SALE_FEE_DEFAULT;
}

// accounts
QStringList account_list() {
    auto database = db();
    QSqlQuery query(database);
    bool ok = query.exec("SELECT name FROM accounts");
    return resultToList(query);
}

bool account_add(const QString &account) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = query.prepare("INSERT OR IGNORE INTO accounts(name) VALUES( ? )");
    query.addBindValue(account);
    ok = query.exec();
    return ok;
}

//funds
qint64 fund(const QString &account) {
    auto database = db();
    QSqlQuery query(database);
    query.exec("SELECT fund FROM accounts WHERE name = ?");
    query.addBindValue(account);
    query.exec();
    query.next();
    return query.value(0).toInt();
}

bool fund_add(const QString &account, qint64 amount) {
    auto database = db();
    QSqlQuery query(database);
    query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=?");
    query.addBindValue(amount);
    query.addBindValue(account);
    bool ok = query.exec();
    return ok;
}

//items
QStringList item_list(const QString &account) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = query.exec("SELECT name FROM items");
    QStringList res;
    while(query.next())
        res << query.value(0).toString();
    return res;
}

bool item_add(const QString &account, QLatin1StringView item) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = query.prepare("INSERT OR IGNORE INTO items(name, owner) VALUES( ?, ? )");
    query.addBindValue(item);
    query.addBindValue(account);
    ok = query.exec();
    return ok;
}

bool item_del(const QString &account, QLatin1StringView item) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = query.prepare("DELETE FROM items WHERE name = ? and account = ?");
    query.addBindValue(item);
    query.addBindValue(account);
    ok = query.exec();
    return ok;
}

//sales
QStringList sale_list(const QString &account) {
    auto database = db();
    QSqlQuery query(database);
    if(account.isEmpty()) {
        query.prepare("SELECT * FROM ITEMS WHERE " DB_FILTER_SALE);
    } else {
        query.prepare("SELECT * FROM ITEMS WHERE owner = ? AND " DB_FILTER_SALE);
        query.addBindValue(account);
    }
    bool ok = query.exec();
    return resultToList(query);
}

bool sale_add(qint64 timestamp, const QString &account, QLatin1StringView item, qint64 price) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = true;
    ok = database.transaction();
    ok = query.prepare("UPDATE items SET "
                            "timestamp = ?, price = ? "
                            " WHERE name = ? AND owner = ? AND "
                            DB_FILTER_NOT_AUCTION_NOT_SALE);
    query.addBindValue(timestamp);
    query.addBindValue(price);
    query.addBindValue(item);
    query.addBindValue(account);

    ok = query.exec();
    ok = 1 == query.numRowsAffected();
    query.finish();

    if (ok) {
        QSqlQuery query(database);
        query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=? and (fund + ?) > 0");
        query.addBindValue(-1 * sale_fee());
        query.addBindValue(account);
        query.addBindValue(-1 * sale_fee());
        ok = query.exec();
        ok = 1 == query.numRowsAffected();
    }


    if(ok) {
        ok = database.commit();
    } else {
        database.rollback();
    }
    return ok;
}

QPair<QString, QString> sale_buy(qint64 timestamp, const QString &account, QLatin1StringView item) {
    auto database = db();
    QSqlQuery query_pay(database);
    QSqlQuery query(database);

    bool ok = true;

    ok = database.transaction();

    ok = query.prepare(
                "UPDATE items SET "
                " owner_prev = owner, price_prev = price, "
                " owner = ?, buyer = NULL, price = NULL, timestamp = NULL "
                " WHERE name = ? and owner != ? AND timestamp < ? AND " DB_FILTER_SALE
                " RETURNING * ");
    query.addBindValue(account);
    query.addBindValue(item);
    query.addBindValue(account);
    query.addBindValue(timestamp - SALE_TIMEOUT);

    ok = query.exec();
    ok = query.next();
    if (!ok) {
        qWarning() << "Couldn't buy item; ; failed to find matching item!" << timestamp << account << item;
    }

    qint64 price = ok ? query.value("price_prev").toInt() : 0;
//    QString buyer = query.value("buyer").toString();
    QString buyer = ok ? account : QString();
    QString owner = ok ? query.value("owner_prev").toString() : QString();
    query.finish();

    if(ok) {
        QSqlQuery query(database);
        query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=? and (fund + ?) > 0");
        query.addBindValue(-1 * price);
        query.addBindValue(buyer);
        query.addBindValue(-1 * price);
        ok = query.exec();
        ok = 1 == query.numRowsAffected();
    }
    if(ok) {
        QSqlQuery query(database);
        query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=? and (fund + ?) > 0");
        query.addBindValue(price);
        query.addBindValue(owner);
        query.addBindValue(price);
        ok = query.exec();
        ok = 1 == query.numRowsAffected();
    }

    if(ok) {
        ok = database.commit();
    } else {
        database.rollback();
        qWarning() << "Couldn't buy item; failed to transfer price!" << timestamp << account << item;
    }
    if(!ok) {
        return QPair<QString, QString>();
    }
    const QString fmt = "'%1' Bought Item '%2' for '%3'.";
    QString msg = fmt.arg(buyer).arg(item).arg(price);
    return QPair<QString, QString>(owner, msg);
}


//auction
QStringList auction_list(const QString &account) {
    auto database = db();
    QSqlQuery query(database);
    if(account.isEmpty()) {
        query.prepare("SELECT * FROM ITEMS WHERE " DB_FILTER_AUCTION);
    } else {
        query.prepare("SELECT * FROM ITEMS WHERE owner = ? AND " DB_FILTER_AUCTION);
        query.addBindValue(account);
    }
    bool ok = query.exec();
    return resultToList(query);
}

bool auction_add(qint64 timestamp, const QString &account, QLatin1StringView item, qint64 price) {
    auto database = db();
    QSqlQuery query(database);
    bool ok = true;
    ok = query.prepare("UPDATE items SET "
                            "timestamp = ?, price = ?, buyer = owner "
                            " WHERE name = ? AND owner = ? AND "
                            DB_FILTER_NOT_AUCTION_NOT_SALE);
    query.addBindValue(timestamp);
    query.addBindValue(price - 1); // to allow a bid equal to starting price to be accepted
    query.addBindValue(item);
    query.addBindValue(account);

    ok = query.exec();
    ok = 1 == query.numRowsAffected();
    qDebug() << "AUCTION by " << account << " on item : " << item << " starting at " << price  << ok;
    return ok;
}

bool auction_bid(qint64 timestamp, const QString &buyer, QLatin1StringView item, qint64 bid) {
    auto database = db();

    bool ok = true;
    ok = database.transaction();

    QSqlQuery query_pay(database);
    QSqlQuery query(database);

    ok = query.prepare(
                "UPDATE items SET "
                " owner_prev = buyer, price_prev = price, "
                " buyer = ?, price = ?, timestamp = ? "
                " WHERE name = ? and owner != ? AND timestamp > ? AND "
                " ? > price AND " DB_FILTER_AUCTION
                " RETURNING * ");
    query.addBindValue(buyer);
    query.addBindValue(bid);
    query.addBindValue(timestamp);
    query.addBindValue(item);
    query.addBindValue(buyer);
    query.addBindValue(timestamp - SALE_TIMEOUT);
    query.addBindValue(bid);
    ok = query.exec();
    if(!query.next()) {
        qWarning() << "Couldn't bid on item; ; failed to find matching item!" << timestamp << buyer << item;
        ok = false;
    }

    qint64 prev_bid = query.value("price_prev").toInt();
    QString prev_bidder = query.value("owner_prev").toString();
    QString owner = query.value("owner").toString();
    query.finish();

    //refund old bid
    if(ok && prev_bidder != owner) {
        QSqlQuery query(database);
        query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=?");
        query.addBindValue(prev_bid);
        query.addBindValue(buyer);
        ok = query.exec();
        ok = 1 == query.numRowsAffected();
    }
    if(ok) {
        QSqlQuery query(database);
        query.prepare("UPDATE accounts SET fund = fund + ? WHERE name=? and (fund + ?) > 0");
        query.addBindValue(-1 * bid);
        query.addBindValue(buyer);
        query.addBindValue(-1 * bid);
        ok = query.exec();
        ok = 1 == query.numRowsAffected();
    }


    if(ok) {
        ok = database.commit();
    } else {
        database.rollback();
        qWarning() << "Couldn't buy item; failed to transfer price!" << timestamp << buyer << item;
    }
    return ok;
}

QHash<QString, QString> sale_conclude_auctions() {
    auto database = db();
    QSqlQuery query(database);

    bool ok = query.prepare(
                "UPDATE items SET "
                " price_prev = price, price = NULL, "
                " owner_prev = owner, owner = buyer, "
                " buyer = NULL, timestamp = NULL "
                " WHERE timestamp + ? <= ?  AND " DB_FILTER_AUCTION
                " RETURNING *"
    );
    query.addBindValue(SALE_TIMEOUT + SALE_TIMEOUT_MARGIN);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    ok = query.exec();
    const QString fmt = "'%1' Bought Item '%2' for '%3'.";
    QHash<QString, QString> res;
    while(query.next()) {
        res[query.value(5).toString()] = fmt.arg(query.value(1).toString()).arg(query.value(0).toString()).arg(query.value(6).toInt());
    }
    return res;
}

QHash<QString, QString> sale_conclude_expired() {
    auto database = db();
    QSqlQuery query(database);

    bool ok = query.prepare(
                "UPDATE items SET "
                " price = NULL, "
                " buyer = NULL, timestamp = NULL "
                " WHERE timestamp + ? <= ?  AND "
                " (buyer = owner OR buyer IS NULL) AND "
                DB_FILTER_SALE_AUCTION
                " RETURNING *"
    );
    query.addBindValue(SALE_TIMEOUT + SALE_TIMEOUT_MARGIN);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    ok = query.exec();
    const QString fmt = "'%1' : Item '%2' sale expired!: for '%3'.";
    QHash<QString, QString> res;
    while(query.next()) {
        res[query.value(5).toString()] = fmt.arg(query.value(1).toString()).arg(query.value(0).toString()).arg(query.value(6).toInt());
    }
    return res;
}

QHash<QString, QString> sale_conclude() {
    sale_conclude_expired();
    return sale_conclude_auctions();
}

}


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

namespace db {

#define DB_NAME_DEFAULT "auction.sqlite3";

#define DB_FILTER_AUCTION " price != 0 AND buyer IS NOT NULL "
#define DB_FILTER_SALE " price != 0 AND buyer IS NULL "
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
    bool ok = query.prepare("UPDATE items SET "
                            "timestamp = ?, price = ? "
                            " WHERE name = ? AND owner = ? AND "
                            DB_FILTER_NOT_AUCTION_NOT_SALE);
    query.addBindValue(timestamp);
    query.addBindValue(price);
    query.addBindValue(item);
    query.addBindValue(account);

    ok = query.exec();
    ok = 1 == query.numRowsAffected();

    if (ok) {

        ok = fund_add(account, -1 * sale_fee());

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

bool sale_buy(qint64 timestamp, const QString &account, QLatin1StringView item) {
    auto database = db();
    QSqlQuery query_pay(database);
    QSqlQuery query(database);

    bool ok = true;

    ok = database.transaction();

    ok = query.prepare("SELECT * FROM items WHERE name = ? and owner != ? AND timestamp < (? - ?) AND " DB_FILTER_SALE);
    query.addBindValue(item);
    query.addBindValue(account);
    query.addBindValue(timestamp);
    query.addBindValue(SALE_TIMEOUT);

    ok = query.exec();
    if(!query.next()) {
        database.rollback();
        qWarning() << "Couldn't buy item; ; failed to find matching item!" << timestamp << account << item;
        return false;
    }

    auto record = query.record();
    qint64 price = record.value("price").toInt();
//    QString buyer = record.value("buyer").toString();
    QString buyer = account;
    QString owner = record.value("owner").toString();

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

    if(!ok) {
        database.rollback();
        qWarning() << "Couldn't buy item; failed to transfer price!" << timestamp << account << item << record;
        return false;
    }

    ok = query.prepare("UPDATE items SET "
                       " owner = ?, buyer = NULL, price = NULL, timestamp = NULL "
                       " WHERE name = ? ");
    query.addBindValue(buyer);
    query.addBindValue(item);
    query.exec();

    if(1 != query.numRowsAffected()) {
        database.rollback();
        qWarning() << "Couldn't buy item; failed to change owner!" << timestamp << account << item;
        return false;
    }

    database.commit();
    return true;
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
    return true;
}

bool auction_bid(qint64 timestamp, const QString &buyer, QLatin1StringView item, qint64 bid) {
    return true;
}

QHash<QString, QString> sale_conclude() {
    auto database = db();
    QSqlQuery query(database);

    bool ok = query.prepare(
                "UPDATE items SET "
                " price_prev = price, price = NULL, "
                " owner_prev = owner, owner = buyer, "
                " buyer = NULL, timestamp = NULL "
                " WHERE timestamp + ? >= UNIXEPOCH()  AND " DB_FILTER_AUCTION
                " RETURNING *"
    );
    query.addBindValue(SALE_TIMEOUT);
    ok = query.exec();
    const QString fmt = "'%1' Bought Item '%2' for '%3'.";
    QHash<QString, QString> res;
    while(query.next()) {
        res[query.value(5).toString()] = fmt.arg(query.value(1).toString()).arg(query.value(0).toString()).arg(query.value(6).toString());
    }
    return res;
}

}


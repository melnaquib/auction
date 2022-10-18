#pragma once

#include <QStringList>

namespace db {

//Logic for items is that;
// bidder not empty; // Item not on sale, may be
// bidder same as owner, item put to sale, no bid yet
//price not zero, item on sale, depending on bidder field being not empty might be on auction

//owner_prev to overcome update returning limitationon sqlite

const QStringList DDL = []() -> QStringList {

QStringList res;

res <<
"\n CREATE TABLE IF NOT EXISTS 'accounts' ( "
"\n     'name'	VARCHAR(64) NOT NULL, "
"\n     'fund'	INTEGER DEFAULT 0, "
"\n     PRIMARY KEY('name') "
"\n ); "
;

res <<
"\n CREATE TABLE IF NOT EXISTS 'items' ( "
"\n     'name'	VARCHAR(64) NOT NULL, "
"\n     'owner'	VARCHAR(64), "

"\n     'price'	INTEGER, "
"\n     'timestamp'	DATETIME, "

"\n     'buyer'	VARCHAR(64), "

"\n     'owner_prev'	VARCHAR(64), "
"\n     'price_prev'	INTEGER, "

"\n     PRIMARY KEY('name') "
"\n ); "
;

res <<
"\n CREATE INDEX IF NOT EXISTS index_items_owner ON items ( owner ); "
;

return res;
} ();

}

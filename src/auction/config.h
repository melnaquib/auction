#pragma once

#include <QSettings>

#define NEWL "\r\n"

const quint16 SERVER_PORT_DEFAULT = 8080;

const qint64 SALE_FEE_DEFAULT = 10; //
const qint64 AUCTION_RAISE_MIN = 5; //
const qint64 SALE_TIMEOUT_MARGIN = 1 * 1000; // extra timeout to process messages
const qint64 SALE_TIMEOUT_INTERLUDE_MIN = 10 * 1000; // minimal time to elapse before reprocessing timed out sales
//const qint64 SALE_TIMEOUT = 5 * 60 * 1000; // 5 min
const qint64 SALE_TIMEOUT = 30 * 1000; // 30 sec for demoe
//const qint64 SALE_TIMEOUT = 3 * 1000; // 30 sec for demoe


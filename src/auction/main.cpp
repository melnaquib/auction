#include "vendor.h"
#include "config.h"
#include "db.h"
#include "server.h"
#include "ui.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setApplicationName(APP_VERSION);
    app.setOrganizationName(ORG_NAME);

    // settings_init();
    db::init();

    Ui ui(&app);

    quint16 port = SERVER_PORT_DEFAULT;
    Server server(&ui, &app);
    server.listen(QHostAddress::Any, port);

    qInfo() << "Server listenning on " << port;

    return app.exec();
}

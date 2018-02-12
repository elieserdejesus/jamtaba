#include "PrivateServerWindow.h"
#include "ui_PrivateServerWindow.h"
#include "ninjam/server/Server.h"

#include <QDateTime>
#include <QtConcurrent/QtConcurrent>

PrivateServerWindow::PrivateServerWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrivateServerWindow),
    networkUsageTimerID(0)
{
    ui->setupUi(this);

    connect(ui->pushButtonStart, &QPushButton::clicked, this, &PrivateServerWindow::startServer);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &PrivateServerWindow::stopServer);

    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStop->setEnabled(false);

    connect(&upnpManager, &UPnPManager::portOpened, this, &PrivateServerWindow::portOpened);

    connect(&upnpManager, &UPnPManager::portClosed, [=](){
        appendTextInLog("Port closed in your router!");
    });

    connect(&upnpManager, &UPnPManager::errorDetected, this, &PrivateServerWindow::upnpError);

    setServerDetailsVisibility(false);

    ui->labelIPValue->setText(QString());
    ui->labelPortValue->setText(QString());
}

void PrivateServerWindow::upnpError(const QString &error)
{
    appendTextInLog("ERROR in UPnP: " + error);
}

void PrivateServerWindow::portOpened(const QString &localIP, const QString &externalIP)
{
    Q_UNUSED(localIP);
    appendTextInLog("Port opened in your router using UPnP protocol! Other ninjamers can connect in your server!");
    appendTextInLog("Your external IP is " + externalIP);

    ui->labelIPValue->setText(externalIP);
    ui->labelPortValue->setText(QString::number(PREFERRED_PORT));

    setServerDetailsVisibility(true);
}

PrivateServerWindow::~PrivateServerWindow()
{

    server.reset(nullptr);

    delete ui;
}

void PrivateServerWindow::setupServer()
{
    if (!server)
        return;

    connect(server.data(), &Server::serverStarted, this, &PrivateServerWindow::serverStarted);
    connect(server.data(), &Server::serverStopped, this, &PrivateServerWindow::serverStopped);

    connect(server.data(), &Server::errorStartingServer, [=](const QString &errorMessage){
        appendTextInLog(errorMessage);
    });

    connect(server.data(), &Server::incommingConnection, [=](const QString &ip){
        appendTextInLog(QString("Incomming connection from %1").arg(ip));
    });

    connect(server.data(), &Server::userEntered, [=](const QString &userName){
        appendTextInLog(QString("%1 entered in the server").arg(userName));

        updateUserList();
    });

    connect(server.data(), &Server::userLeave, [=](const QString &userName){
        appendTextInLog(QString("%1 left the server").arg(userName));

        updateUserList();
    });


}

void PrivateServerWindow::appendTextInLog(const QString &text)
{
    QString now = QDateTime::currentDateTime().toUTC().toString("hh:mm:ss");
    ui->textBrowser->append(QString("[%1] %2").arg(now).arg(text));
}

void PrivateServerWindow::updateUserList()
{
    if (!server)
        return;

    ui->listWidget->clear();
    for (auto userName : server->getConnectedUsersNames()) {
        ui->listWidget->addItem(userName);
    }
}

void PrivateServerWindow::serverStarted()
{
    ui->textBrowser->append(QString());
    appendTextInLog("Server started");

    ui->pushButtonStart->setEnabled(false);
    ui->pushButtonStop->setEnabled(true);

    ui->listWidget->clear();

    if (networkUsageTimerID)
        killTimer(networkUsageTimerID);

    networkUsageTimerID = startTimer(3000);
}

void PrivateServerWindow::serverStopped()
{
    appendTextInLog("Server stopped!");

    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStop->setEnabled(false);

    ui->labelIPValue->setText(QString());
    ui->labelPortValue->setText(QString());

    setServerDetailsVisibility(false);

    ui->listWidget->clear();

    if (networkUsageTimerID)
        killTimer(networkUsageTimerID);
}

void PrivateServerWindow::setServerDetailsVisibility(bool visible)
{
    ui->labelIP->setVisible(visible);
    ui->labelPort->setVisible(visible);
    ui->labelIPValue->setVisible(visible);
    ui->labelPortValue->setVisible(visible);

    ui->labelDownload->setVisible(visible);
    ui->labelDownloadValue->setVisible(visible);
    ui->labelUpload->setVisible(visible);
    ui->labelUploadValue->setVisible(visible);
}

void PrivateServerWindow::startServer()
{
    if (!server) {
        server.reset(new Server());
        setupServer();
    }

    if (!server->isStarted()) {
        quint16 port = PREFERRED_PORT;
        server->start(port);

        appendTextInLog(QString("Trying to open the port %1 in your router (UPnP) to allow external connections ...").arg(port));

        QtConcurrent::run(&upnpManager, &UPnPManager::openPort, port);
    }
}

void PrivateServerWindow::stopServer()
{
    if (!server)
        return;

    if (server->isStarted()) {
        server->shutdown();

        QtConcurrent::run(&upnpManager, &UPnPManager::closePort, PREFERRED_PORT);
    }
}

void PrivateServerWindow::timerEvent(QTimerEvent *)
{
    if (!server)
        return;

    quint64 download = server->getDownloadTransferRate() / 1024 * 8; // Kbps
    quint64 upload = server->getUploadTransferRate() / 1024 * 8;
    ui->labelDownloadValue->setText(QString::number(download));
    ui->labelUploadValue->setText(QString::number(upload));
}

#ifndef _SERVER_SERVER_
#define _SERVER_SERVER_

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QList>

#include "../Ninjam.h"
#include "ninjam/client/User.h"

namespace ninjam { namespace client {
    class ClientToServerChatMessage;
    class UserChannel;
}}

namespace ninjam {

namespace server {

using ninjam::client::User;
using ninjam::client::UserChannel;
using ninjam::client::ClientToServerChatMessage;

class RemoteUser : public User
{
public:
    RemoteUser();
    void setLastKeepAliveToNow();
    quint64 getLastKeepAliveReceived() const;
    MessageHeader getCurrentHeader() const;
    void setCurrentHeader(MessageHeader header);
    void setFullName(const QString &fullName);
    void updateChannels(const QList<UserChannel> &newChannels);

    inline bool receivedInitialServerInfos() const
    {
        return receivedServerInfos;
    }

    inline void setReceivedServerInfos()
    {
        receivedServerInfos = true;
    }

private:
    MessageHeader currentHeader;
    quint64 lastKeepAliveReceived;
    bool receivedServerInfos;
};

inline void RemoteUser::setCurrentHeader(MessageHeader header)
{
    currentHeader = header;
}

inline MessageHeader RemoteUser::getCurrentHeader() const
{
    return currentHeader;
}

inline quint64 RemoteUser::getLastKeepAliveReceived() const
{
    return lastKeepAliveReceived;
}

class Server : public QObject
{
    Q_OBJECT

public:
    Server();
    virtual ~Server();
    virtual void start(quint16 port);
    void shutdown();

    bool isStarted() const;

    quint16 getPort() const;
    QString getIP() const;

    quint16 getBpi() const;
    quint16 getBpm() const;
    QString getTopic() const;
    QString getLicence() const;
    quint8 getMaxUsers() const;
    quint8 getMaxChannels() const;

    QStringList getConnectedUsersNames() const;

signals:
    void serverStarted();
    void serverStopped();
    void incommingConnection(const QString &incommingIP);
    void userEntered(const QString &userName);
    void userLeave(const QString &userName);

protected:
    void sendAuthChallenge(QTcpSocket *device);

protected slots:
    virtual void handleNewConnection();
    void handleAcceptError(QAbstractSocket::SocketError socketError);
    void processReceivedBytes();
    void handleDisconnection();
    void handleClientSocketError(QAbstractSocket::SocketError error);

private:
    QTcpServer tcpServer;
    QMap<QTcpSocket *, RemoteUser> remoteUsers; // connected clients

    quint16 bpm;
    quint16 bpi;
    QString topic;
    QString licence;
    quint8 maxUsers;
    quint8 maxChannels;
    quint16 keepAlivePeriod;

    void broadcastUserChanges(const QString userFullName, const QList<UserChannel> &userChannels);
    void sendConnectedUsersTo(QTcpSocket *socket);
    void broadcastPublicChatMessage(const ClientToServerChatMessage &receivedMessage, const QString &userFullName);
    void broadcastServerMessage(const QString &serverMessage);
    void broadcastServerMessage(const QString &serverMessage, QTcpSocket *exclude);

    void processClientAuthUserMessage(QTcpSocket *socket, const MessageHeader &header);
    void processClientSetChannel(QTcpSocket *socket, const MessageHeader &header);
    void processUploadIntervalBegin(QTcpSocket *socket, const MessageHeader &header);
    void processUploadIntervalWrite(QTcpSocket *socket, const MessageHeader &header);
    void processChatMessage(QTcpSocket *socket, const MessageHeader &header);
    void processKeepAlive(QTcpSocket *socket, const MessageHeader &header);
    void processClientSetUserMask(QTcpSocket *socket, const MessageHeader &header);

    void sendServerInitialInfosTo(QTcpSocket *socket);

    void sendPrivateMessage(const QString &sender, const ClientToServerChatMessage &receivedMessage);
    void processAdminCommand(const QString &cmd);
    void setTopic(const QString &newTopic);
    void setBpi(quint16 newBpi);
    void setBpm(quint16 newBpm);

    void disconnectClient(QTcpSocket *socket);

    QString generateUniqueUserName(const QString &userName) const; // return sanitized and unique username

    void updateKeepAliveInfos();

    static QHostAddress getBestHostAddress();
};

inline quint8 Server::getMaxChannels() const
{
    return maxChannels;
}

inline quint8 Server::getMaxUsers() const
{
    return maxUsers;
}

inline QString Server::getLicence() const
{
    return licence;
}

inline QString Server::getTopic() const
{
    return topic;
}

inline quint16 Server::getBpi() const
{
    return bpi;
}

inline quint16 Server::getBpm() const
{
    return bpm;
}

} // ns server
} // ns ninjam

#endif

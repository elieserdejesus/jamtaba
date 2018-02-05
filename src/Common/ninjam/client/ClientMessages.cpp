#include "ClientMessages.h"
#include "ninjam/client/User.h"
#include "ninjam/Ninjam.h"

#include <QCryptographicHash>
#include <QIODevice>
#include <QDebug>
#include <QDataStream>

using ninjam::client::ClientMessage;
using ninjam::client::ClientAuthUserMessage;
using ninjam::client::ClientIntervalUploadWrite;
using ninjam::client::ClientKeepAlive;
using ninjam::client::ClientSetChannel;
using ninjam::client::ClientSetUserMask;
using ninjam::client::ClientUploadIntervalBegin;
using ninjam::client::ChatMessage;

ClientMessage::ClientMessage(quint8 msgCode, quint32 payload) :
    msgType(msgCode),
    payload(payload)
{
    //
}

ClientMessage::~ClientMessage()
{
    //
}

//++++++++++++++++++++++++++++++++++++++=
//class ClientAuthUser
/*
     //message type 0x80

     Offset Type        Field
     0x0    uint8_t[20] Password Hash (binary hash value)
     0x14   ...         Username (NUL-terminated)
     x+0x0  uint32_t    Client Capabilities
     x+0x4  uint32_t    Client Version

     The Password Hash field is calculated by SHA1(SHA1(username + ":" + password) + challenge).
     If the user acknowledged a license agreement from the server then Client Capabilities bit 0 is set.
     The server responds with Server Auth Reply.

    //message lenght = 20 bytes password hash + user name lengh + 4 bytes client capabilites + 4 bytes client version
*/

ClientAuthUserMessage::ClientAuthUserMessage(const QString &userName, const QByteArray &challenge, quint32 protocolVersion, const QString &password)
    : ClientMessage(0x80, 0),
      userName(userName),
      clientCapabilites(1),
      protocolVersion(protocolVersion),
      challenge(challenge)
{
    if (!password.isNull() && !password.isEmpty()) {
        this->userName = userName;
    }
    else{
        this->userName = "anonymous:" + userName;
    }

    QCryptographicHash sha1(QCryptographicHash::Sha1);
    sha1.addData( userName.toStdString().c_str(), userName.size() );
    sha1.addData(":", 1);
    sha1.addData(password.toStdString().c_str(), password.size());
    QByteArray passHash = sha1.result();
    sha1.reset();
    sha1.addData(passHash);
    sha1.addData(challenge.constData(), 8);
    this->passwordHash = sha1.result();
    this->payload = 29 + this->userName.size();
}

ClientAuthUserMessage ClientAuthUserMessage::unserializeFrom(QIODevice *device, quint32 payload)
{
    Q_UNUSED(payload)

    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    QByteArray passwordHash(20, Qt::Uninitialized);
    int readed = stream.readRawData(passwordHash.data(), passwordHash.size());
    Q_ASSERT(readed = passwordHash.size());

    QString userName = ninjam::extractString(stream);

    QByteArray challenge(8, Qt::Uninitialized);
    readed = stream.readRawData(challenge.data(), challenge.size());
    Q_ASSERT(readed = challenge.size());

    quint32 clientCapabilites;
    quint32 protocolVersion;

    stream >> clientCapabilites;
    stream >> protocolVersion;

    return ClientAuthUserMessage(userName, challenge, protocolVersion, QString(passwordHash));

}

void ClientAuthUserMessage::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType << payload;
    ninjam::serializeByteArray(passwordHash, stream);
    ninjam::serializeString(userName, stream);
    stream << clientCapabilites;
    stream << protocolVersion;
}

void ClientAuthUserMessage::printDebug(QDebug &dbg) const
{
    dbg << "SEND ClientAuthUserMessage{  userName:" << userName << " challenge:" << challenge <<"}" << endl;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

ClientSetChannel::ClientSetChannel(const QStringList &channels) :
    ClientMessage(0x82, 0),
    volume(0),
    pan(0),
    flags(0)
{
    payload = 2;
    channelNames.append(channels);
    for (int i = 0; i < channelNames.size(); i++) {
        payload += (channelNames[i].toUtf8().size() + 1) + 2 + 1 + 1; // NUL + volume(short) + pan(byte) + flags(byte)
    }
}


ClientSetChannel::ClientSetChannel(const QString &channelNameToRemove) :
    ClientMessage(0x82, 0),
    volume(0),
    pan(0),
    flags(1) //to remove
{
    payload = 2;
    channelNames.append(channelNameToRemove);
    for (int i = 0; i < channelNames.size(); i++) {
        payload += (channelNames[i].toUtf8().size() + 1) + 2 + 1 + 1; // NUL + volume(short) + pan(byte) + flags(byte)
    }
}

ClientSetChannel ClientSetChannel::unserializeFrom(QIODevice *device, quint32 payload)
{
    if (payload <= 0)
        return ClientSetChannel(QStringList()); // no channels

    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint16 channelParameterSize; // ??
    stream >> channelParameterSize;

    quint32 bytesConsumed = sizeof(quint16);

    QStringList channelNames;

    while (bytesConsumed < payload && !stream.atEnd()) {

        quint16 volume;
        quint8 pan;
        quint8 flags;

        QString channelName = ninjam::extractString(stream);

        stream >> volume;
        stream >> pan;
        stream >> flags;

        bytesConsumed += channelName.toUtf8().size() + 1;
        bytesConsumed += sizeof(volume) + sizeof(pan) + sizeof(flags);

        channelNames.append(channelName);
    }

    return ClientSetChannel(channelNames);
}

void ClientSetChannel::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType << payload;
    //++++++++
    stream << quint16(4); // parameter size (4 bytes - volume (2 bytes) + pan (1 byte) + flags (1 byte))
    for (int i = 0; i < channelNames.size(); ++i) {
        serializeString(channelNames[i], stream);
        stream << volume;
        stream << pan;
        stream << flags;
    }
}

void ClientSetChannel::printDebug(QDebug &dbg) const
{
    dbg << "SEND ClientSetChannel{ payloadLenght="
        << payload
        << " channelName="
        << channelNames
        << '}'
        << endl;
}

//+++++++++++++++++++++

ClientKeepAlive::ClientKeepAlive() :
    ClientMessage(0xfd, 0)
{

}

void ClientKeepAlive::serializeTo(QIODevice *device) const
{
    // just the header bytes, no payload
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType;
    stream << payload;
}

void ClientKeepAlive::printDebug(QDebug &dbg) const
{
    dbg << "SEND {Client KeepAlive}"
        << endl;
}

//+++++++++++++++++

ClientSetUserMask::ClientSetUserMask(const QString &userName, quint32 channelsMask) :
    ClientMessage(0x81, 0),
    userName(userName),
    channelsMask(channelsMask)
{
    payload = 4; // 4 bytes (int) flag
    payload += userName.size() + 1;
}

void ClientSetUserMask::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType;
    stream << payload;

    //++++++++++++  END HEADER ++++++++++++

    ninjam::serializeString(userName, stream);
    stream << channelsMask;
}

void ClientSetUserMask::printDebug(QDebug &dbg) const
{
    dbg << "SEND ClientSetUserMask{ userName="
        << userName
        << " flag="
        << channelsMask
        << '}';
}

//+++++++++++++++++++++++++++++

ChatMessage::ChatMessage(const QString &text, ChatMessageType type) :
    ClientMessage(0xc0, 0),
    text(ChatMessage::satinizeText(text, type)),
    command(getTypeCommand(type)),
    type(type)
{
    payload = this->text.toUtf8().size() + 1 + command.length() + 1;
}

QString ChatMessage::satinizeText(const QString &msg, ChatMessageType type)
{
    if (type == ChatMessageType::AdminMessage) {
        return msg.right(msg.size() - 1); // remove the first char (/)
    }
    else if (type == ChatMessageType::PrivateMessage) { // remove '/msg ' from string
        return QString(msg).replace(QString("/msg "), "");
    }

    return msg;
}

QString ChatMessage::getTypeCommand(ChatMessageType type)
{
    switch (type) {
        case ChatMessageType::AdminMessage:   return "ADMIN";
        case ChatMessageType::PrivateMessage: return "PRIVMSG";
        case ChatMessageType::TopicMessage:   return "TOPIC";
    }

    return "MSG";
}

void ChatMessage::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType;
    stream << payload;

    ninjam::serializeString(command, stream);
    if (type != ChatMessageType::PrivateMessage) {
        ninjam::serializeString(text, stream);
    }
    else {
        QChar whiteSpace(' ');
        if (text.contains(whiteSpace)) {
            int whiteSpaceIndex = text.indexOf(whiteSpace);
            QString userFullName = text.left(whiteSpaceIndex);
            QString message = text.mid(whiteSpaceIndex + 1);

            ninjam::serializeString(userFullName, stream);
            ninjam::serializeString(message, stream);
        }
    }
}

void ChatMessage::printDebug(QDebug &dbg) const
{
    dbg << "SEND ChatMessage{ payload: "
        << payload
        << " "
        << "command="
        << command
        << " text="
        << text
        << '}';
}

//+++++++++++++++++++++++++

ClientUploadIntervalBegin::ClientUploadIntervalBegin(const QByteArray &GUID, quint8 channelIndex, const QString &userName, bool isAudioInterval) :
    ClientMessage( 0x83, 16 + 4 + 4 + 1 + userName.size()),
    GUID(GUID),
    estimatedSize(0),
    channelIndex(channelIndex),
    userName(userName)
{
    if (isAudioInterval) {
        this->fourCC[0] = 'O';
        this->fourCC[1] = 'G';
        this->fourCC[2] = 'G';
        this->fourCC[3] = 'v';
    }
    else {
        // JamTaba Video prefix
        this->fourCC[0] = 'J';
        this->fourCC[1] = 'T';
        this->fourCC[2] = 'B';
        this->fourCC[3] = 'v';
    }
}

void ClientUploadIntervalBegin::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    //quint32 payload = 16 + 4 + 4 + 1 + userName.size();

    stream << msgType;
    stream << payload;
    stream.writeRawData(GUID, 16);
    stream << estimatedSize;
    stream.writeRawData(fourCC, 4);
    stream << channelIndex;
    stream.writeRawData(userName.toStdString().c_str(), userName.size());
}

void ClientUploadIntervalBegin::printDebug(QDebug &dbg) const
{
    dbg << "SEND ClientUploadIntervalBegin{ GUID "
        << QString(GUID)
        << " fourCC"
        << QString(fourCC)
        << "channelIndex: "
        << channelIndex
        << "userName:"
        << userName
        << "}";
}

//+++++++++++++++++++++

ClientIntervalUploadWrite::ClientIntervalUploadWrite(const QByteArray &GUID, const QByteArray &encodedData, bool isLastPart) :
    ClientMessage(0x84, 16 + 1 + encodedData.size()),
    GUID(GUID),
    encodedData(encodedData),
    isLastPart(isLastPart)
{

}

void ClientIntervalUploadWrite::serializeTo(QIODevice *device) const
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << msgType;
    stream << payload;

    stream.writeRawData(GUID.data(), 16);
    quint8 intervalCompleted = isLastPart ? (quint8) 1 : (quint8) 0; // If the Flag field bit 0 is set then the upload is complete.
    stream << intervalCompleted;
    stream.writeRawData(encodedData.data(), encodedData.size());
}

void ClientIntervalUploadWrite::printDebug(QDebug &dbg) const
{
    dbg << "SEND ClientIntervalUploadWrite{"
        << "GUID="
        << QString(GUID)
        << ", encodedAudioBuffer= "
        << payload
        << " bytes, isLastPart="
        << isLastPart
        << '}';
}

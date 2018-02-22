#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <QScopedPointer>
#include <QImage>

#include "UploadIntervalData.h"
#include "geo/IpToLocationResolver.h"
#include "loginserver/LoginService.h"
#include "persistence/Settings.h"
#include "persistence/UsersDataCache.h"
#include "audio/core/AudioMixer.h"
#include "midi/MidiDriver.h"
#include "video/FFMpegMuxer.h"
#include "gui/chat/EmojiManager.h"

class MainWindow;

namespace ninjam { namespace client {
class Service;
class ServerInfo;
class User;
}}

namespace persistence {
class LocalInputTrackSettings;
class Preset;
}

namespace audio {
class AudioNode;
class LocalInputNode;
class LocalInputGroup;
class AudioPeak;
class SamplesBuffer;
class AbstractMp3Streamer;
}

namespace recorder {
class JamRecorder;
}

namespace controller {

class NinjamController;

using ninjam::client::Service;
using ninjam::client::ServerInfo;
using ninjam::client::User;
using persistence::Settings;
using persistence::LocalInputTrackSettings;
using persistence::Preset;
using persistence::UsersDataCache;
using audio::AudioNode;
using audio::AudioPeak;
using audio::LocalInputNode;
using audio::LocalInputGroup;
using audio::SamplesBuffer;
using audio::AbstractMp3Streamer;
using audio::AudioMixer;
using login::RoomInfo;
using login::LoginService;
using recorder::JamRecorder;

class MainController : public QObject
{
    Q_OBJECT

    friend class controller::NinjamController;

protected:
    explicit MainController(const Settings &settings);

public:

    virtual ~MainController();

    virtual void start();
    virtual void stop();

    long getTotalUploadTransferRate() const;
    long getTotalDownloadTransferRate() const;
    long getDownloadTransferRate(const QString userFullName, quint8 channelIndex) const;

    void setVideoProperties(const QSize &resolution);

    QSize getVideoResolution() const;

    void setFullScreenView(bool fullScreen);

    virtual QString getJamtabaFlavor() const = 0;     // return Standalone or VstPlugin

    virtual void setMainWindow(MainWindow *mainWindow);

    virtual std::vector<midi::MidiMessage> pullMidiMessagesFromPlugins() = 0;     // pull midi messages generated by plugins. This function can be called many times in each audio processing cicle because every VSTi can be a midi messages generator, and we need get the generated messages after call the plugin 'process' function.

    void saveLastUserSettings(const LocalInputTrackSettings &inputsSettings);

    // presets
    virtual Preset loadPreset(const QString &name);     // one preset
    QStringList getPresetList();     // return all presets names
    void savePreset(const LocalInputTrackSettings &inputsSettings,
                    const QString &name);
    void deletePreset(const QString &name);     // not used yet

    // main audio processing routine
    virtual void process(const SamplesBuffer &in, SamplesBuffer &out, int sampleRate);

    void sendNewChannelsNames(const QStringList &channelsNames);
    void sendRemovedChannelMessage(int removedChannelIndex);

    inline QString getTheme() const
    {
        return settings.getTheme();
    }

    bool addTrack(long trackID, AudioNode *trackNode);
    void removeTrack(long trackID);

    void playRoomStream(const RoomInfo &roomInfo);
    bool isPlayingRoomStream() const;

    bool isPlayingInNinjamRoom() const;
    virtual void stopNinjamController();

    void setTransmitingStatus(int channelID, bool transmiting);
    bool isTransmiting(int channelID) const;

    void stopRoomStream();     // stop currentRoom stream
    long long getCurrentStreamingRoomID() const;

    int getLastTracksLayout() const;

    void storeTracksLayoutOrientation(quint8 layout);

    void storeTracksSize(bool usingNarrowedTracks);

    bool isUsingNarrowedTracks() const;

    void storeWaveDrawingMode(quint8 drawingMode);

    quint8 getLastWaveDrawingMode() const;

    void enterInRoom(const RoomInfo &room, const QStringList &channelsNames,
                     const QString &password = "");

    LoginService *getLoginService() const;

    void connectInJamtabaServer();

    virtual controller::NinjamController *getNinjamController() const;

    Service *getNinjamService();

    static QStringList getBotNames();

    // tracks
    void setTrackMute(int trackID, bool muteStatus, bool blockSignals = false);
    bool trackIsMuted(int trackID) const;
    void setTrackSolo(int trackID, bool soloStatus, bool blockSignals = false);
    bool trackIsSoloed(int trackID) const;
    void setTrackGain(int trackID, float gain, bool blockSignals = false);
    void setTrackBoost(int trackID, float boostInDecibels);
    void setTrackPan(int trackID, float pan, bool blockSignals = false);
    void resetTrack(int trackID);     // reset mute, solo, gain, pan, etc
    void setTrackStereoInversion(int trackID, bool stereoInverted);
    bool trackStereoIsInverted(int trackID) const;

    AudioPeak getRoomStreamPeak();
    AudioPeak getTrackPeak(int trackID);
    AudioPeak getMasterPeak();

    float getMasterGain() const;

    void setMasterGain(float newGain);

    AudioNode *getTrackNode(long ID);

    bool isStarted() const;

    geo::Location getGeoLocation(const QString &ip);

    LocalInputNode *getInputTrack(int localInputIndex);
    virtual int addInputTrackNode(LocalInputNode *inputTrackNode);
    void removeInputTrackNode(int inputTrackIndex);
    void removeAllInputTracks();

    LocalInputNode *getInputTrackInGroup(quint8 groupIndex, quint8 trackIndex) const;

    int getInputTracksCount() const;
    int getInputTrackGroupsCount() const;

    void mixGroupedInputs(int groupIndex, SamplesBuffer &out);

    virtual float getSampleRate() const = 0;

    float getEncodingQuality() const;

    static QByteArray newGUID();

    const Settings &getSettings() const;

    // store settings
    void storeMetronomeSettings(float metronomeGain, float metronomePan, bool metronomeMuted);
    void storeIntervalProgressShape(int shape);

    void storeWindowSettings(bool maximized, const QPointF &location, const QSize &size);
    void storeIOSettings(int firstIn, int lastIn, int firstOut, int lastOut, int audioDevice,
                         const QList<bool> &midiInputStatus);

    void storeMultiTrackRecordingStatus(bool savingMultiTracks);
    bool isMultiTrackRecordingActivated() const;
    void storeMultiTrackRecordingPath(const QString &newPath);

    void storeJamRecorderStatus(const QString &writerId, bool status);

    bool isJamRecorderActivated(const QString &writerId) const;

    void storePrivateServerSettings(const QString &server, int serverPort, const QString &password);

    bool isUsingCustomMetronomeSounds() const;

    void setTranslationLanguage(const QString &languageCode);
    inline QString getTranslationLanguage() const
    {
        return settings.getTranslation();
    }

    void setBuiltInMetronome(const QString &metronomeAlias);
    void setCustomMetronome(const QString &primaryBeatFile, const QString &offBeatFile,
                            const QString &accentBeatFile);

    QString getMetronomeFirstBeatFile() const;
    QString getMetronomeOffBeatFile() const;
    QString getMetronomeAccentBeatFile() const;

    void saveEncodedAudio(const QString &userName, quint8 channelIndex,
                          const QByteArray &encodedAudio);

    AbstractMp3Streamer *getRoomStreamer() const;

    void setUserName(const QString &newUserName);

    QString getUserName() const;

    bool userNameWasChoosed() const;

    // used to recreate audio encoder with enough channels
    int getMaxAudioChannelsForEncoding(uint trackGroupIndex) const;

    bool setTheme(const QString &themeName);

    // TODO: move this code to NinjamController.
    void finishUploads();     // used to send the last part of ninjam intervals when audio is stopped.

    virtual QString getUserEnvironmentString() const;

    // to remembering ninjamers controls (pan, level, gain, boost)
    UsersDataCache *getUsersDataCache();     // TODO hide this from callers. Create a function in mainController to update the CacheEntries, so MainController is used as a Façade.

    bool userIsBlockedInChat(const QString &userName) const;

    void storeMeteringSettings(bool showingMaxPeaks, quint8 meterOption);

    static QString getSuggestedUserName();

    QMap<QString, QString> getJamRecoders() const;

    void setChannelReceiveStatus(const QString &userFullName, quint8 channelIndex,
                                 bool receiveChannel);

    // looper settings
    void storeLooperPreferredLayerCount(quint8 layersCount);
    void storeLooperPreferredMode(quint8 looperMode);
    void storeLooperAudioEncodingFlag(bool encodeAudioWhenSaving);
    void storeLooperFolder(const QString &newLooperFolder);
    quint8 getLooperPreferedLayersCount() const;
    quint8 getLooperPreferedMode() const;
    bool getLooperAudioEncodingFlag() const;
    quint8 getLooperBitDepth() const;

    void setAllLoopersStatus(bool activated);

    // collapse settings
    void setLocalChannelsCollapsed(bool collapsed);
    void setBottomSectionCollapsed(bool collapsed);
    void setChatSectionCollapsed(bool collapsed);
    bool isLocalChannelsCollapsed() const;
    bool isBottomSectionCollapsed() const;
    bool isChatSectionCollapsed() const;

    static bool crashedInLastExecution();
    static QString getVersionFromLogContent();

    EmojiManager *getEmojiManager() const;

    qint8 getChatFontSizeOffset() const;

signals:
    void ipResolved(const QString &ip);
    void themeChanged();
    void userBlockedInChat(const QString &userName);
    void userUnblockedInChat(const QString &userName);

public slots:
    virtual void setSampleRate(int newSampleRate);
    void setEncodingQuality(float newEncodingQuality);
    void storeLooperBitDepth(quint8 bitDepth);

    void storeRemoteUserRememberSettings(bool boost, bool level, bool pan, bool mute, bool lowCut);
    void storeCollapsibleSectionsRememberSettings(bool localChannels, bool bottomSection,
                                                  bool chatSection);

    void blockUserInChat(const QString &userNameToBlock);
    void unblockUserInChat(const QString &userNameToUnblock);

    void processCapturedFrame(int frameID, const QImage &frame);

    virtual void connectInNinjamServer(const ServerInfo &server);

    void storeChatFontSizeOffset(qint8 fontSizeOffset);

protected:

    static QString LOG_CONFIG_FILE;

    LoginService loginService;

    AudioMixer audioMixer;

    // ninjam
    QScopedPointer<Service> ninjamService;
    QScopedPointer<controller::NinjamController> ninjamController;

    Settings settings;

    QMap<int, LocalInputNode *> inputTracks;

    virtual controller::NinjamController *createNinjamController() = 0;

    MainWindow *mainWindow;

    // map the input channel indexes to a GUID (used to upload audio to ninjam server)
    QMap<quint8, UploadIntervalData> audioIntervalsToUpload;
    UploadIntervalData videoIntervalToUpload;

    QMutex mutex;

    virtual void setupNinjamControllerSignals();

    virtual void setCSS(const QString &css) = 0;

    virtual std::vector<midi::MidiMessage> pullMidiMessagesFromDevices() = 0;     // pull midi messages generated by midi controllers. This function is called just one time in each audio processing cicle.

    // audio process is here too (see MainController::process)
    virtual void doAudioProcess(const SamplesBuffer &in, SamplesBuffer &out,
                                int sampleRate);

    virtual void syncWithNinjamIntervalStart(uint intervalLenght);

    FFMpegMuxer videoEncoder;

private:
    void setAllTracksActivation(bool activated);

    QScopedPointer<AbstractMp3Streamer> roomStreamer;
    long long currentStreamingRoomID;

    QMap<int, LocalInputGroup *> trackGroups;

    QMap<int, bool> getXmitChannelsFlags() const;

    QMap<long, AudioNode *> tracksNodes;

    bool started;

    void tryConnectInNinjamServer(const RoomInfo &ninjamRoom, const QStringList &channels,
                                  const QString &password = "");

    QScopedPointer<geo::IpToLocationResolver> ipToLocationResolver;

    QList<JamRecorder *> jamRecorders;

    QList<JamRecorder *> getActiveRecorders() const;

    // master
    float masterGain;
    AudioPeak masterPeak;

    UsersDataCache usersDataCache;

    int lastInputTrackID;     // used to generate a unique key/ID for each input track

    const static quint8 CAMERA_FPS;

    bool canGrabNewFrameFromCamera() const;

    quint64 lastFrameTimeStamp;

    void recreateMetronome();

    uint getFramesPerInterval() const;

    static const QString CRASH_FLAG_STRING;

    void enqueueAudioDataToUpload(const QByteArray &encodedData, quint8 channelIndex, bool isFirstPart);
    void enqueueVideoDataToUpload(const QByteArray &encodedData, quint8 channelIndex, bool isFirstPart);

    EmojiManager emojiManager;

    QSet<QString> chatBlockedUsers;

protected slots:

    // ninjam
    virtual void disconnectFromNinjamServer(const ServerInfo &server);
    virtual void quitFromNinjamServer(const QString &error);

    virtual void enqueueDataToUpload(const QByteArray &encodedData, quint8 channelIndex, bool isFirstPart);

    virtual void updateBpi(int newBpi);
    virtual void updateBpm(int newBpm);

    // TODO move this slot to NinjamController
    virtual void handleNewNinjamInterval();

    void requestCameraFrame(int intervalPosition);

    void uploadEncodedVideoData(const QByteArray &encodedData, bool firstPart);
};

inline void MainController::setLocalChannelsCollapsed(bool collapsed)
{
    settings.setLocalChannelsCollapsed(collapsed);
}

inline void MainController::setBottomSectionCollapsed(bool collapsed)
{
    settings.setBottomSectionCollapsed(collapsed);
}

inline void MainController::setChatSectionCollapsed(bool collapsed)
{
    settings.setChatSectionCollapsed(collapsed);
}

inline bool MainController::isLocalChannelsCollapsed() const
{
    return settings.isLocalChannelsCollapsed();
}

inline bool MainController::isBottomSectionCollapsed() const
{
    return settings.isBottomSectionCollapsed();
}

inline bool MainController::isChatSectionCollapsed() const
{
    return settings.isChatSectionCollapsed();
}

inline qint8 MainController::getChatFontSizeOffset() const
{
    return settings.getChatFontSizeOffset();
}

inline EmojiManager *MainController::getEmojiManager() const
{
    return const_cast<EmojiManager *>(&emojiManager);
}

inline UsersDataCache *MainController::getUsersDataCache()
{
    return &usersDataCache;
}

inline bool MainController::userNameWasChoosed() const
{
    return !settings.getUserName().isEmpty();
}

inline AbstractMp3Streamer *MainController::getRoomStreamer() const
{
    return roomStreamer.data();
}

inline QString MainController::getMetronomeFirstBeatFile() const
{
    return settings.getMetronomeFirstBeatFile();
}

inline QString MainController::getMetronomeOffBeatFile() const
{
    return settings.getMetronomeOffBeatFile();
}

inline QString MainController::getMetronomeAccentBeatFile() const
{
    return settings.getMetronomeAccentBeatFile();
}

inline bool MainController::isUsingCustomMetronomeSounds() const
{
    return settings.isUsingCustomMetronomeSounds();
}

inline bool MainController::isJamRecorderActivated(const QString &writerId) const
{
    return settings.isJamRecorderActivated(writerId);
}

inline bool MainController::isMultiTrackRecordingActivated() const
{
    return settings.isSaveMultiTrackActivated();
}

inline const Settings &MainController::getSettings() const
{
    return settings;
}

inline float MainController::getEncodingQuality() const
{
    return settings.getEncodingQuality();
}

inline int MainController::getInputTracksCount() const
{
    return inputTracks.size();     // return the individual tracks (subchannels) count
}

inline int MainController::getInputTrackGroupsCount() const
{
    return trackGroups.size();     // return the track groups (channels) count
}

inline bool MainController::isStarted() const
{
    return started;
}

inline AudioPeak MainController::getMasterPeak()
{
    return masterPeak;
}

inline float MainController::getMasterGain() const
{
    return masterGain;
}

inline controller::NinjamController *MainController::getNinjamController() const
{
    return ninjamController.data();
}

inline Service *MainController::getNinjamService()
{
    return ninjamService.data();
}

inline long long MainController::getCurrentStreamingRoomID() const
{
    return currentStreamingRoomID;
}

inline int MainController::getLastTracksLayout() const
{
    return settings.getLastTracksLayoutOrientation();
}

inline void MainController::storeTracksLayoutOrientation(quint8 layout)
{
    settings.storeTracksLayoutOrientation(layout);
}

inline void MainController::storeTracksSize(bool usingNarrowedTracks)
{
    settings.storeTracksSize(usingNarrowedTracks);
}

inline bool MainController::isUsingNarrowedTracks() const
{
    return settings.isUsingNarrowedTracks();
}

inline void MainController::storeWaveDrawingMode(quint8 drawingMode)
{
    settings.storeWaveDrawingMode(drawingMode);
}

inline quint8 MainController::getLastWaveDrawingMode() const
{
    return settings.getLastWaveDrawingMode();
}

inline void MainController::setMainWindow(MainWindow *mainWindow)
{
    this->mainWindow = mainWindow;
}

inline void MainController::storeLooperBitDepth(quint8 bitDepth)
{
    settings.setLooperBitDepth(bitDepth);
}

inline quint8 MainController::getLooperBitDepth() const
{
    return settings.getLooperBitDepth();
}

inline void MainController::storeLooperPreferredLayerCount(quint8 layersCount)
{
    settings.setLooperPreferredLayersCount(layersCount);
}

inline void MainController::storeLooperPreferredMode(quint8 looperMode)
{
    settings.setLooperPreferredMode(looperMode);
}

inline void MainController::storeLooperAudioEncodingFlag(bool encodeAudioWhenSaving)
{
    settings.setLooperAudioEncodingFlag(encodeAudioWhenSaving);
}

inline void MainController::storeLooperFolder(const QString &newLooperFolder)
{
    settings.setLooperFolder(newLooperFolder);
}

inline quint8 MainController::getLooperPreferedLayersCount() const
{
    return settings.getLooperPreferredLayersCount();
}

inline quint8 MainController::getLooperPreferedMode() const
{
    return settings.getLooperPreferredMode();
}

inline bool MainController::getLooperAudioEncodingFlag() const
{
    return settings.getLooperAudioEncodingFlag();
}

inline void MainController::storeRemoteUserRememberSettings(bool boost, bool level, bool pan,
                                                            bool mute, bool lowCut)
{
    settings.setRemoteUserRememberingSettings(boost, level, pan, mute, lowCut);
}

inline void MainController::storeCollapsibleSectionsRememberSettings(bool localChannels,
                                                                     bool bottomSection,
                                                                     bool chatSection)
{
    settings.setCollapsileSectionsRememberingSettings(localChannels, bottomSection, chatSection);
}
} // namespace

#endif

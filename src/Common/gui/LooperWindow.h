#ifndef LOOPERWINDOW_H
#define LOOPERWINDOW_H

#include <QDialog>

#include "audio/AudioLooper.h"
#include "LooperWavePanel.h"

namespace Controller {
class NinjamController;
}

namespace Ui {
class LooperWindow;
}

class LooperWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LooperWindow(QWidget *parent);
    ~LooperWindow();

    void setLooper(Audio::Looper *looper, Controller::NinjamController *controller);
    void detachCurrentLooper();
    void updateDrawings();

protected:
    void paintEvent(QPaintEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void updateBeatsPerInterval();
    void updateCurrentBeat(uint currentIntervalBeat);
    void updateControls();
    void updateLayersVisibility(quint8 newMaxLayers);

private:
    Ui::LooperWindow *ui;
    Audio::Looper *looper;
    QMap<quint8, LooperWavePanel*> wavePanels;
    Controller::NinjamController *controller;

    void deleteWavePanels();

    void addSamplesPeak(float peak, uint samplesCount, quint8 layerIndex);

    void initializeControls();

    int currentBeat;
};

#endif // LOOPERWINDOW_H
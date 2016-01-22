#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include <QDebug>
#include <QFileDialog>
#include "persistence/Settings.h"

using namespace Audio;

PreferencesDialog::PreferencesDialog(MainWindow *mainWindow) :
    QDialog(mainWindow),
    ui(new Ui::IODialog),
    mainWindow(mainWindow)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & Qt::WA_DeleteOnClose);

    ui->comboLastOutput->setEnabled(false);
}

void PreferencesDialog::initialize(int initialTab, const Persistence::RecordingSettings &recordingSettings)
{
    Q_UNUSED(initialTab);
    this->recordingSettings = recordingSettings;
    setupSignals();
    populateAllTabs();
}

void PreferencesDialog::setupSignals()
{
    // the 'accept' slot is overrided in inherited classes (StandalonePreferencesDialog and VstPreferencesDialog)
    connect(ui->okButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
    connect(ui->prefsTab, SIGNAL(currentChanged(int)), this, SLOT(selectTab(int)));
    connect(ui->recordingCheckBox, SIGNAL(clicked(bool)), this,
            SIGNAL(multiTrackRecordingStatusChanged(bool)));
    connect(ui->browseRecPathButton, SIGNAL(clicked(bool)), this, SLOT(selectRecordingPath()));
}

void PreferencesDialog::populateAllTabs()
{
    populateRecordingTab();
}

void PreferencesDialog::selectRecordingTab()
{
    ui->prefsTab->setCurrentWidget(ui->tabRecording);
}

void PreferencesDialog::populateRecordingTab()
{
    QDir recordDir(recordingSettings.recordingPath);
    ui->recordingCheckBox->setChecked(recordingSettings.saveMultiTracksActivated);
    ui->recordPathLineEdit->setText(recordDir.absolutePath());
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::selectRecordingPath()
{
    QFileDialog fileDialog(this, "Choosing recording path ...");
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::DirectoryOnly);
    if (fileDialog.exec()) {
        QDir dir = fileDialog.directory();
        QString newRecordingPath = dir.absolutePath();
        ui->recordPathLineEdit->setText(newRecordingPath);
        emit recordingPathSelected(newRecordingPath);
    }
}

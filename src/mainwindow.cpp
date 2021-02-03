#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

// BAlert API
#include <Windows.h>
#include "AbmSdkInclude.h"
#pragma comment(lib, "BAlert.lib")
// LSL API
#include <lsl_cpp.h>


// some constant device properties
const char *const name_by_type[] = {"None","X10","X24","X4"};
int channels_by_type[] = {0,10,24,4};
const int sRate = 256;


MainWindow::MainWindow(QWidget *parent, const char *config_file)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// make GUI connections
	connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
	connect(ui->actionLoad_Configuration, &QAction::triggered, [this]() {
		load_config(QFileDialog::getOpenFileName(
			this, "Load Configuration File", "", "Configuration Files (*.cfg)"));
	});
	connect(ui->actionSave_Configuration, &QAction::triggered, [this]() {
		save_config(QFileDialog::getSaveFileName(
			this, "Save Configuration File", "", "Configuration Files (*.cfg)"));
	});
	connect(ui->actionAbout, &QAction::triggered, [this]() {
		QString infostr = QStringLiteral("LSL library version: ") +
						  QString::number(lsl::library_version()) +
						  "\nLSL library info:" + lsl::library_info();
		QMessageBox::about(this, "About this app", infostr);
	});
	connect(ui->linkButton, &QPushButton::clicked, this, &MainWindow::link_balert);

	// parse startup config file
	QString cfgfilepath = find_config_file(config_file);
	load_config(cfgfilepath);
}

void MainWindow::load_config(const QString &filename) {
	QSettings settings(filename, QSettings::Format::IniFormat);
	ui->useFilter->setCheckState(settings.value("BAlert/usefilter", true).toBool() ? Qt::Checked : Qt::Unchecked);
}

//: Save function, same as above
void MainWindow::save_config(const QString &filename) {
	QSettings settings(filename, QSettings::Format::IniFormat);
	settings.beginGroup("BAlert");
	settings.setValue("usefilter", ui->useFilter->checkState() == Qt::Checked);
	settings.sync();
}

void MainWindow::closeEvent(QCloseEvent *ev) {
	if (reader) {
		QMessageBox::warning(this, "Still reading from device.", "Can't quit while linked");
		ev->ignore();
	}
		
}

// background data reader thread
void reader_thread_function(std::atomic<bool> &shutdown) {

	_DEVICE_INFO *pInfo = GetDeviceInfo(NULL);
	if (pInfo == NULL) { throw std::runtime_error("No device found. Is it plugged in?"); }

	// TODO: Determine the device type from pInfo. Below is hard-coded to X24Standard.
	int init_res = InitSession(ABM_DEVICE_X24Standard, ABM_SESSION_RAW, -1, 0);
	if (init_res != INIT_SESSION_OK) {
		throw std::runtime_error("Could not initialize ABM session.");
	}

	// TODO: What's better? pInfo->nNumberOfChannel or the following?
	int nChannels, nDeconPacketChannelsNmb, nPSDPacketChannelsNmb,
		nRawPSDPacketChannelNmb, nQualityPacketChannelNmb;
	int pkt_chan_res = GetPacketChannelNmbInfo(nChannels, nDeconPacketChannelsNmb,
		nPSDPacketChannelsNmb, nRawPSDPacketChannelNmb, nQualityPacketChannelNmb);

	// create streaminfo
	std::string modelname = std::string("BAlert");  // TOOD: Get device name from pInfo
	lsl::stream_info info(
		modelname, "EEG", nChannels + 6, sRate, lsl::cf_float32, modelname + "_sdetsff");
	// append some meta-data
	info.desc()
		.append_child("acquisition")
		.append_child_value("manufacturer", "Advanced Brain Monitoring")
		.append_child_value("model", modelname.c_str());
	lsl::xml_element chans_info = info.desc().append_child("channels");
	chans_info.append_child("channel").append_child_value("label", "Epoch");
	chans_info.append_child("channel").append_child_value("label", "Offset");
	chans_info.append_child("channel").append_child_value("label", "Hour");
	chans_info.append_child("channel").append_child_value("label", "Min");
	chans_info.append_child("channel").append_child_value("label", "Sec");
	chans_info.append_child("channel").append_child_value("label", "MilliSec");
	// TODO: Handle channel names from other device types.
	std::vector<std::string> chan_names = {"Fp1", "F7", "F8", "T4", "T6", "T5", "T3", "Fp2", "O1", "P3", "Pz", "F3", "Fz", "F4", "C4", "P4", "POz", "C3", "Cz", "O2", "EKG", "AUX1", "AUX2", "AUX3"};
	for (size_t i = 0; i < nChannels; i++) {
		chans_info.append_child("channel").append_child_value("label", chan_names[i]);
	}

	// make a new outlet
	lsl::stream_outlet outlet(info);

	// try to connect
	int start_res = StartAcquisition();
	if (start_res != ACQ_STARTED_OK) {
		throw std::runtime_error("Could not start ABM acquisition.");
	}
	
	// enter transmission loop
	int nReceived;
	while (!shutdown) {
		float *pData;
		pData = GetRawData(nReceived);
		if (pData) {
			// pData size is (nChannels+6)*nReceived
			for (size_t k = 0; k < nReceived; k++) { outlet.push_sample(&pData[k*(nChannels+6)]);
			}
		}
	}
	StopAcquisition();
}

// start/stop the BAlert connection
void MainWindow::link_balert() {
	if (reader) {

		// === perform unlink action ===
		try {
			shutdown = true;
			// reader->interrupt();
			reader->join();
			reader.reset();
		} catch(std::exception &e) {
			QMessageBox::critical(this,"Error",(std::string("Could not stop the background processing: ")+=e.what()).c_str(),QMessageBox::Ok);
			return;
		}
		// indicate that we are now successfully unlinked
		ui->linkButton->setText("Link");
	} else {
		// === perform link action ===
		bool connected = false;
		try {

			// get the UI parameters...
			bool usefilter = ui->useFilter->checkState()==Qt::Checked;

			connected = true;

			// start reading
			shutdown = false;
			reader = std::make_unique<std::thread>(&reader_thread_function, std::ref(shutdown));
		}
		catch(std::exception &e) {
			if (connected) StopAcquisition();
			QMessageBox::critical(this,"Error",(std::string("Could not initialize the BAlert interface: ")+=e.what()).c_str(),QMessageBox::Ok);
			return;
		}

		// done, all successful
		ui->linkButton->setText("Unlink");
	}
}

/**
 * Find a config file to load. This is (in descending order or preference):
 * - a file supplied on the command line
 * - [executablename].cfg in one the the following folders:
 *	- the current working directory
 *	- the default config folder, e.g. '~/Library/Preferences' on OS X
 *	- the executable folder
 * @param filename	Optional file name supplied e.g. as command line parameter
 * @return Path to a found config file
 */
QString MainWindow::find_config_file(const char *filename) {
	if (filename) {
		QString qfilename(filename);
		if (!QFileInfo::exists(qfilename))
			QMessageBox(QMessageBox::Warning, "Config file not found",
				QStringLiteral("The file '%1' doesn't exist").arg(qfilename), QMessageBox::Ok,
				this);
		else
			return qfilename;
	}
	QFileInfo exeInfo(QCoreApplication::applicationFilePath());
	QString defaultCfgFilename(exeInfo.completeBaseName() + ".cfg");
	QStringList cfgpaths;
	cfgpaths << QDir::currentPath()
			 << QStandardPaths::standardLocations(QStandardPaths::ConfigLocation) << exeInfo.path();
	for (auto path : cfgpaths) {
		QString cfgfilepath = path + QDir::separator() + defaultCfgFilename;
		if (QFileInfo::exists(cfgfilepath)) return cfgfilepath;
	}
	QMessageBox(QMessageBox::Warning, "No config file not found",
		QStringLiteral("No default config file could be found"), QMessageBox::Ok, this);
	return "";
}

MainWindow::~MainWindow() noexcept = default;

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <atomic>
#include <memory>
#include<thread>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
	explicit MainWindow(QWidget *parent, const char *config_file);
	~MainWindow() noexcept override;

private slots:
	void closeEvent(QCloseEvent *ev);
	void link_balert(); // start the BAlert connection

private:
	// function for loading / saving the config file
	QString find_config_file(const char *filename);
	void load_config(const QString &filename);
	void save_config(const QString &filename);

	std::atomic<bool> shutdown{false};			 // whether we're trying to stop the reader thread
	std::unique_ptr<std::thread> reader; // our reader thread
	std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H

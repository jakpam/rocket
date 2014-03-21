#include "mainwindow.h"
#include "trackview.h"
#include "syncdocument.h"

extern "C" {
#include "../lib/sync.h"
#include "../lib/track.h"
}

#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QFileInfo>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
typedef int socklen_t;
#endif

MainWindow::MainWindow(SOCKET serverSocket) :
	QMainWindow(),
	guiConnected(false),
	serverSocket(serverSocket),
	clientIndex(0)
{
	setWindowTitle("GNU Rocket System");

	trackView = new TrackView(this);
	setCentralWidget(trackView);

	connect(trackView, SIGNAL(posChanged()),
	        this, SLOT(onPosChanged()));
	connect(trackView, SIGNAL(currValDirty()),
	        this, SLOT(onCurrValDirty()));

	createMenuBar();
	createStatusBar();

	startTimer(0);
}

void MainWindow::createMenuBar()
{
	fileMenu = menuBar()->addMenu("&File");
	fileMenu->addAction("New", this, SLOT(fileNew()), Qt::CTRL + Qt::Key_N);
	fileMenu->addAction("&Open", this, SLOT(fileOpen()), Qt::CTRL + Qt::Key_O);
	fileMenu->addAction("&Save", this, SLOT(fileSave()), Qt::CTRL + Qt::Key_S);
	fileMenu->addAction("Save &As", this, SLOT(fileSaveAs()));
	fileMenu->addSeparator();
	fileMenu->addAction("Remote &Export", this, SLOT(fileRemoteExport()), Qt::CTRL + Qt::Key_E);
	recentFilesMenu = fileMenu->addMenu("Recent &Files");
	for (int i = 0; i < 5; ++i) {
		recentFileActions[i] = recentFilesMenu->addAction("");
		recentFileActions[i]->setVisible(false);
		connect(recentFileActions[i], SIGNAL(triggered()),
		        this, SLOT(openRecentFile()));
	}
	updateRecentFiles();
	fileMenu->addSeparator();
	fileMenu->addAction("E&xit", this, SLOT(fileQuit()));

	editMenu = menuBar()->addMenu("&Edit");
	editMenu->addAction("Undo", trackView, SLOT(editUndo()), Qt::CTRL + Qt::Key_Z);
	editMenu->addAction("Redo", trackView, SLOT(editRedo()), Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
	editMenu->addSeparator();
	editMenu->addAction("&Copy", trackView, SLOT(editCopy()), Qt::CTRL + Qt::Key_C);
	editMenu->addAction("Cu&t", trackView, SLOT(editCut()), Qt::CTRL + Qt::Key_X);
	editMenu->addAction("&Paste", trackView, SLOT(editPaste()), Qt::CTRL + Qt::Key_V);
	editMenu->addAction("Clear", trackView, SLOT(editClear()), Qt::Key_Delete);
	editMenu->addSeparator();
	editMenu->addAction("Select All", trackView, SLOT(selectAll()), Qt::CTRL + Qt::Key_A);
	editMenu->addAction("Select Track", trackView, SLOT(selectTrack()), Qt::CTRL + Qt::Key_T);
	editMenu->addAction("Select Row", trackView, SLOT(selectRow()));
	editMenu->addSeparator();
	editMenu->addAction("Bias Selection", this, SLOT(editBiasSelection()), Qt::CTRL + Qt::Key_B);
	editMenu->addSeparator();
	editMenu->addAction("Set Rows", this, SLOT(editSetRows()), Qt::CTRL + Qt::Key_R);
	editMenu->addSeparator();
	editMenu->addAction("Previous Bookmark", this, SLOT(editPreviousBookmark()), Qt::ALT + Qt::Key_PageUp);
	editMenu->addAction("Next Bookmark", this, SLOT(editNextBookmark()), Qt::ALT + Qt::Key_PageDown);
}

void MainWindow::createStatusBar()
{
	statusText = new QLabel;
	statusRow = new QLabel;
	statusCol = new QLabel;
	statusValue = new QLabel;
	statusKeyType = new QLabel;

	statusBar()->addWidget(statusText);
	statusBar()->addWidget(statusRow);
	statusBar()->addWidget(statusCol);
	statusBar()->addWidget(statusValue);
	statusBar()->addWidget(statusKeyType);

	statusText->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusRow->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusCol->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusValue->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusKeyType->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	setStatusText("Not connected");
	setStatusPosition(0, 0);
	setStatusValue(0.0f, false);
	setStatusKeyType(KEY_STEP, false);
}

static QStringList getRecentFiles()
{
#ifdef WIN32
	QSettings settings("HKEY_CURRENT_USER\\Software\\GNU Rocket",
	                   QSettings::NativeFormat);
#else
	QSettings settings;
#endif
	QStringList list;
	for (int i = 0; i < 5; ++i) {
		QVariant string = settings.value(QString("RecentFile%1").arg(i));
		if (string.isValid())
			list.push_back(string.toString());
	}
	return list;
}

void MainWindow::updateRecentFiles()
{
	QStringList files = getRecentFiles();

	if (!files.size()) {
		recentFilesMenu->setEnabled(false);
		return;
	}

	Q_ASSERT(files.size() <= 5);
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo info(files[i]);
		QString text = QString("&%1 %2").arg(i + 1).arg(info.fileName());

		recentFileActions[i]->setText(text);
		recentFileActions[i]->setData(info.absoluteFilePath());
		recentFileActions[i]->setVisible(true);
	}
	recentFilesMenu->setEnabled(true);
}

void MainWindow::setCurrentFileName(const QString &fileName)
{
	QFileInfo info(fileName);

	QSettings settings;
	QStringList files = getRecentFiles();
	files.removeAll(info.absoluteFilePath());
	files.prepend(info.absoluteFilePath());
	while (files.size() > 5)
		files.removeLast();

	for (int i = 0; i < files.size(); ++i)
		settings.setValue(QString("RecentFile%1").arg(i), files[i]);

	updateRecentFiles();

	setWindowFileName(fileName);
}

void MainWindow::setWindowFileName(const QString &fileName)
{
	QFileInfo info(fileName);
	setWindowTitle(QString("GNU Rocket System - %1").arg(info.fileName()));
}

void MainWindow::setStatusText(const QString &text)
{
	statusText->setText(text);
}

void MainWindow::setStatusPosition(int row, int col)
{
	statusRow->setText(QString::number(row));
	statusCol->setText(QString::number(col));
}

void MainWindow::setStatusValue(double val, bool valid)
{
	if (valid)
		statusValue->setText(QString::number(val, 'f'));
	else
		statusValue->setText("---");
}

void MainWindow::setStatusKeyType(enum key_type keyType, bool valid)
{
	if (!valid) {
		statusKeyType->setText("---");
		return;
	}

	switch (keyType) {
	case KEY_STEP:   statusKeyType->setText("step"); break;
	case KEY_LINEAR: statusKeyType->setText("linear"); break;
	case KEY_SMOOTH: statusKeyType->setText("smooth"); break;
	case KEY_RAMP:   statusKeyType->setText("ramp"); break;
	}
}

void MainWindow::setDocument(SyncDocument *newDoc)
{
	SyncDocument *oldDoc = trackView->getDocument();

	if (oldDoc && oldDoc->clientSocket.connected()) {
		// delete old key frames
		for (size_t i = 0; i < oldDoc->num_tracks; ++i) {
			sync_track *t = oldDoc->tracks[i];
			for (int j = 0; j < t->num_keys; ++j)
				oldDoc->clientSocket.sendDeleteKeyCommand(t->name, t->keys[j].row);
		}

		if (newDoc) {
			// add back missing client-tracks
			std::map<const std::string, size_t>::const_iterator it;
			for (it = oldDoc->clientSocket.clientTracks.begin(); it != oldDoc->clientSocket.clientTracks.end(); ++it) {
				int trackIndex = sync_find_track(newDoc, it->first.c_str());
				if (0 > trackIndex)
					trackIndex = int(newDoc->createTrack(it->first.c_str()));
			}

			// copy socket and update client
			newDoc->clientSocket = oldDoc->clientSocket;

			for (size_t i = 0; i < newDoc->num_tracks; ++i) {
				sync_track *t = newDoc->tracks[i];
				for (int j = 0; j < t->num_keys; ++j)
					newDoc->clientSocket.sendSetKeyCommand(t->name, t->keys[j]);
			}
		}
	}

	trackView->setDocument(newDoc);
	trackView->dirtyCurrentValue();
	trackView->viewport()->update();

	if (oldDoc)
		delete oldDoc;
}

void MainWindow::fileNew()
{
	setDocument(new SyncDocument);
	setWindowFileName("Untitled");
}

void MainWindow::loadDocument(const QString &path)
{
	SyncDocument *newDoc = SyncDocument::load(path);
	if (newDoc) {
		// set new document
		setDocument(newDoc);
		setCurrentFileName(path);
	}
	else
		QMessageBox::critical(this, NULL, QString("failed to open %1").arg(path), QMessageBox::Ok);
}

void MainWindow::fileOpen()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Open File", "", "ROCKET File (*.rocket);;All Files (*.*)");
	if (fileName.length()) {
		loadDocument(fileName);
	}
}

void MainWindow::fileSaveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save File", "", "ROCKET File (*.rocket);;All Files (*.*)");
	if (fileName.length()) {
		SyncDocument *doc = trackView->getDocument();
		if (doc->save(fileName)) {
			doc->clientSocket.sendSaveCommand();
			setCurrentFileName(fileName);
			doc->fileName = fileName;
		} else
			QMessageBox::critical(this, NULL, QString("failed to save %1").arg(fileName), QMessageBox::Ok);
	}
}

void MainWindow::fileSave()
{
	SyncDocument *doc = trackView->getDocument();
	if (doc->fileName.isEmpty())
		return fileSaveAs();

	if (!doc->save(doc->fileName)) {
		doc->clientSocket.sendSaveCommand();
		QMessageBox::critical(this, NULL, QString("failed to save %1").arg(doc->fileName), QMessageBox::Ok);
	}
}

void MainWindow::fileRemoteExport()
{
	trackView->getDocument()->clientSocket.sendSaveCommand();
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		loadDocument(action->data().toString());
}

void MainWindow::fileQuit()
{
	SyncDocument *doc = trackView->getDocument();
	if (doc->modified()) {
		QMessageBox::StandardButton res = QMessageBox::question(
		    NULL, "GNU Rocket", "Save before exit?",
		    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (res == QMessageBox::Yes) {
			fileSave();
			QApplication::quit();
		} else if (res == QMessageBox::No)
			QApplication::quit();
	}
	else QApplication::quit();
}

void MainWindow::editBiasSelection()
{
	bool ok = false;
	float bias = QInputDialog::getDouble(this, "Bias Selection", "", 0, INT_MIN, INT_MAX, 1, &ok);
	if (ok)
		trackView->editBiasValue(bias);
}

void MainWindow::editSetRows()
{
	bool ok = false;
	int rows = QInputDialog::getInt(this, "Set Rows", "", trackView->getRows(), 0, INT_MAX, 1, &ok);
	if (ok)
		trackView->setRows(rows);
}

void MainWindow::editPreviousBookmark()
{
	int row = trackView->getDocument()->prevRowBookmark(trackView->getEditRow());
	if (row >= 0)
		trackView->setEditRow(row);
}

void MainWindow::editNextBookmark()
{
	int row = trackView->getDocument()->nextRowBookmark(trackView->getEditRow());
	if (row >= 0)
		trackView->setEditRow(row);
}

void MainWindow::onPosChanged()
{
	setStatusPosition(trackView->getEditRow(), trackView->getEditTrack());
}

void MainWindow::onCurrValDirty()
{
	SyncDocument *doc = trackView->getDocument();
	if (doc && doc->getTrackCount() > 0) {
		const sync_track *t = doc->tracks[doc->getTrackIndexFromPos(trackView->getEditTrack())];
		int row = trackView->getEditRow();
		setStatusValue(sync_get_val(t, row), true);
		int idx = key_idx_floor(t, row);
		if (idx >= 0)
			setStatusKeyType(t->keys[idx].type, true);
		else
			setStatusKeyType(KEY_STEP, false);
	} else {
		setStatusValue(0.0f, false);
		setStatusKeyType(KEY_STEP, false);
	}
}

void MainWindow::processCommand(ClientSocket &sock)
{
	SyncDocument *doc = trackView->getDocument();
	int strLen, serverIndex, newRow;
	std::string trackName;
	const sync_track *t;
	unsigned char cmd = 0;
	if (sock.recv((char*)&cmd, 1)) {
		switch (cmd) {
		case GET_TRACK:
			// read data
			sock.recv((char *)&strLen, sizeof(int));
			strLen = ntohl(strLen);
			if (!sock.connected())
				return;

			if (!strLen) {
				sock.disconnect();
				trackView->update();
				return;
			}

			trackName.resize(strLen);
			if (!sock.recv(&trackName[0], strLen))
				return;

			if (int(strlen(trackName.c_str())) != strLen) {
				sock.disconnect();
				trackView->update();
				return;
			}

			// find track
			serverIndex = sync_find_track(doc,
			    trackName.c_str());
			if (0 > serverIndex)
				serverIndex =
				    int(doc->createTrack(trackName));

			// setup remap
			doc->clientSocket.clientTracks[trackName] = clientIndex++;

			// send key frames
			t = doc->tracks[serverIndex];
			for (int i = 0; i < (int)t->num_keys; ++i)
				doc->clientSocket.sendSetKeyCommand(trackName,
				    t->keys[i]);

			trackView->update();
			break;

		case SET_ROW:
			sock.recv((char*)&newRow, sizeof(int));
			trackView->setEditRow(ntohl(newRow));
			break;
		}
	}
}

static TcpSocket *clientConnect(SOCKET serverSocket, sockaddr_in *host)
{
	sockaddr_in hostTemp;
	socklen_t hostSize = sizeof(sockaddr_in);
	SOCKET clientSocket = accept(serverSocket, (sockaddr*)&hostTemp, &hostSize);
	if (INVALID_SOCKET == clientSocket)
		return NULL;

	const char *expectedGreeting = CLIENT_GREET;
	std::string line;

	line.resize(0);
	for (;;) {
		char ch;
		if (recv(clientSocket, &ch, 1, 0) != 1) {
			closesocket(clientSocket);
			return NULL;
		}

		if (ch == '\n')
			break;
		if (ch != '\r')
			line.push_back(ch);
		if (ch == '!')
			break;
	}

	TcpSocket *ret = NULL;
	if (!line.compare(0, 4, "GET ")) {
		ret = WebSocket::upgradeFromHttp(clientSocket);
		line.resize(strlen(expectedGreeting));
		if (!ret || !ret->recv(&line[0], line.length())) {
			closesocket(clientSocket);
			return NULL;
		}
	} else
		ret = new TcpSocket(clientSocket);

	if (line.compare(0, strlen(expectedGreeting), expectedGreeting) ||
	    !ret->send(SERVER_GREET, strlen(SERVER_GREET), true)) {
		ret->disconnect();
		return NULL;
	}

	if (NULL != host)
		*host = hostTemp;
	return ret;
}

void MainWindow::timerEvent(QTimerEvent * /* event */)
{
	SyncDocument *doc = trackView->getDocument();
	if (!doc->clientSocket.connected()) {
		if (socket_poll(serverSocket)) {
			setStatusText("Accepting...");
			sockaddr_in client;
			TcpSocket *clientSocket = clientConnect(serverSocket, &client);
			if (clientSocket) {
				setStatusText(QString("Connected to %1").arg(inet_ntoa(client.sin_addr)));
				doc->clientSocket.socket = clientSocket;
				clientIndex = 0;
				doc->clientSocket.sendPauseCommand(true);
				doc->clientSocket.sendSetRowCommand(trackView->getEditRow());
				guiConnected = true;
			} else
				setStatusText("Not Connected.");
		}
	}

	if (doc->clientSocket.connected()) {
		ClientSocket &clientSocket = doc->clientSocket;

		// look for new commands
		while (clientSocket.pollRead())
			processCommand(clientSocket);
	}

	if (!doc->clientSocket.connected() && guiConnected) {
		doc->clientSocket.clientPaused = true;
		trackView->update();
		setStatusText("Not Connected.");
		guiConnected = false;
	}
}

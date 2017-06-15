#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include "audiomodel.h"
/**
 * @brief Konstruktor. Tworzy okno wraz ze wszystkimi przyciskami dla osoby przeprowadzającej konkurs krzykaczy.
 * @param uw Okno z rankingiem uczestników konkursu.
 * @param parent Okno nadrzędne.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
MainWindow::MainWindow(UserWindow *uw, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	setFixedSize(size());
    userWindow = uw;
    recordOnRun = false;
	calibrator = new Calibrator(&recorder, this);
    initialiseDeviceList();

    connect(ui->recordButton, SIGNAL(pressed()), this, SLOT(proceed()));
	connect(ui->deviceComboBox, SIGNAL(currentTextChanged(QString)), &recorder, SLOT(InitialiseRecorder(QString)));
	connect(calibrator, SIGNAL(calibrationStopped()), this, SLOT(onCalibrationStopped()));

    ui->AdminUserList->setColumnCount(5);
    QStringList Header;
    Header<<"Imię"<<"Nazwisko"<<"Płeć"<<"Wynik"<<"Czy było ponowne podejście?";
    ui->AdminUserList->setHorizontalHeaderLabels(Header);
    ui->AdminUserList->horizontalHeader()->setStretchLastSection(true); // Resize last column to fit QTableWidget edge.
    ui->AllRadioButton->setChecked(true);
}
/**
 * @brief Destruktor. Niszczy okno administratora.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk
 */
MainWindow::~MainWindow()
{
	delete ui;
	delete calibrator;
}
/**
 * @brief Metoda wywołująca okno po zamknięciu programu. Umożliwia użytkownikowi powrót do programu bądź zapis listy użytkowników do pliku CSV.
 * @param event zdarzenie kliknięcia przycisku "Zakończ"
 * @authors Dariusz Jóźko
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, tr("Zamykanie programu"), tr("Czy chcesz eksportować listę do pliku przed zamknięciem?"), QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);
	if (resBtn == QMessageBox::Cancel)
	{
		event->ignore();
	}
	else if (resBtn == QMessageBox::Yes)
	{
		on_actionExportToCsv_triggered();
		event->accept();
		userWindow->close();
	}
	else
	{
		event->accept();
		userWindow->close();
	}
}
/**
 * @brief Metoda odpowiadająca za przycisk "Nagrywaj". Uruchamia proces pobierania danych z urządzenia wejścia i przetwarzania go na głośność krzyku wyrażaną w decybelach.
 * @warning Może zostać wyłącznie wywołana, gdy wybrany jest jeden z uczestników, w przeciwnym wypadku wyświetli adekwatny błąd.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::proceed()
{
    int rowindex = ui->AdminUserList->selectionModel()->currentIndex().row();
	try
    {    if (rowindex >= 0)
         {
            if (ui->AdminUserList->item(rowindex,4)->checkState() == Qt::Unchecked)
            {
                if (ui->AdminUserList->item(rowindex,3)->data(Qt::DisplayRole).toDouble() != 0.0)
                {
                    ui->AdminUserList->item(rowindex,4)->setCheckState(Qt::Checked);
                }
				if (!recordOnRun)
                {
					connect(&recorder, SIGNAL(recordingStopped(const QVector<std::complex<double> > &)), this, SLOT(onRecordingStopped(const QVector<std::complex<double> > &)));
					currentUser = rowindex; // onRecordingStopped() slot must know, to which user it should assigns shout level.
                    recorder.Start();
                    recordOnRun = true;
                    ui->recordButton->setText(tr("Zatrzymaj"));
                    ui->deviceComboBox->setEnabled(false);
                }
                else
                {
                    recorder.Stop();
				}
            }
            else
            {
                QMessageBox::information(this, windowTitle(), tr("Ten użytkownik wyczerpał już swój limit podejść."));
            }
        }
        else
        {
            QMessageBox::information(this, windowTitle(), tr("Zaznacz użytkownika, od którego chcesz pobrać próbkę krzyku."));
        }
	}
	catch (exception &e)
	{
		QMessageBox::critical(this, windowTitle(), e.what());
		return;
	}
}
/**
 * @brief Metoda wywołana po 5 sekundach od rozpoczęcia nagrywania. Przypisuje wynik do aktualnie wybranego użytkownika i wyświetla użytkownika wraz z wynikiem na oknie przeznaczonym dla publiczności.\
 * @param complexData Dane pobrane podczas nagrywania z urządzenia wejścia
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::onRecordingStopped(const QVector<std::complex<double> > &complexData)
{
    qDebug() << Calibrator::calibrationData;
    double result = AudioModel::computeLevel(complexData, Calibrator::calibrationData);

	User::setShoutScore(currentUser, result);
	userWindow->InsertUserToRanking(User::GetUser(currentUser), currentUser);
	ui->AdminUserList->setItem(currentUser,3,new QTableWidgetItem(QString::number(result))); // Update shout score in adminWindow's table.
	ui->recordButton->setText(tr("Nagrywaj"));
	ui->deviceComboBox->setEnabled(true);
	recordOnRun = false;
	disconnect(&recorder, 0, this, 0); // Prevent mainWindow from receiving signals from recorder.
}
/**
 * @brief Metoda kończąca kalibrację. Udostępnia możliwość kliknięcia przycisku "Nagrywaj" bądź wybrania urządzenia wejścia.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::onCalibrationStopped()
{
	ui->deviceComboBox->setEnabled(true);
	ui->recordButton->setEnabled(true);
}
/**
 * @brief Metoda odpowiedzialna za możliwość wyboru urządzenia wejścia.
 * @warning Wybór nie jest możliwy gdy trwa nagrywanie lub kalibracja.
 * @warning Gdy nie zostanie wykryte żadne urządzenie wejścia, zostanie wyświetlony adekwatny błąd.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::initialiseDeviceList()
{
    auto devices = recorder.GetAvailableDevices();
    if (devices.isEmpty())
    {
        // Set 'Nagrywaj' button and device list as disabled, so that user could not interact with them.
        ui->deviceComboBox->setEnabled(false);
        ui->recordButton->setEnabled(false);
        QMessageBox::critical(this, windowTitle(), tr("Nie znaleziono żadnych urządzeń do nagrywania. Sprawdź swoje ustawienia i uruchom program ponownie."));
    }
    else
    {
        ui->deviceComboBox->addItems(devices);
    }
}
/**
 * @brief Metoda odpowiedzialna za dodanie nowego uczestnika konkursu do listy uczestników w oknie prowadzącego konkurs.
 * @param user Nowy użytkownik.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::insertUserToList(User * const user)
{
    int row = ui->AdminUserList->rowCount() - 1;
    ui->AdminUserList->setItem(row, 0, new QTableWidgetItem(user->getFirstName()));
    ui->AdminUserList->setItem(row, 1, new QTableWidgetItem(user->getLastName()));
    QString g = user->getPersonGender() == man ? "M" : "K";
    ui->AdminUserList->setItem(row, 2, new QTableWidgetItem(g));
    ui->AdminUserList->setItem(row, 3, new QTableWidgetItem(QString::number(user->getShoutScore())));

    auto *checkBoxCell = new QTableWidgetItem(); // Need to assign QTableWidgetItem's address to a pointer in order to call next two functions
    // Do not worry about 'new' operator, QTableWidget can handle this.
    checkBoxCell->data(Qt::CheckStateRole);
    checkBoxCell->setCheckState(Qt::Unchecked);
	ui->AdminUserList->setItem(row, 4, checkBoxCell);
}
/**
 * @brief Metoda odpowiedzialna za działanie przycisku "Dodaj użytkownika". Uruchamia nowe okno pozwalające wpisać dane uczestnika, a następnie dodaje go do rankingu w oknie dla publiczności.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski Dariusz Jóźko
 */
void MainWindow::on_AddUserButton_clicked()
{
   auw = new AddUserWindow(this);
   if (auw->exec() == 1) // exec returns 1 when accepted.
   {
       User U(auw->GetName(), auw->GetSurName(), auw->GetGender(),0);
       ui->AdminUserList->setRowCount(ui->AdminUserList->rowCount()+1);
       insertUserToList(&U);
   }
   delete auw;
}
/**
 * @brief Metoda odpowiedzialna za działanie przycisku "Edytuj użytkownika". Uruchamia nowe okno pozwalające edytować dane wybranego użytkownika uczestnika, a następnie aktualizuje dane uczestnika w oknie dla publiczności.
 * @warning Gdy żaden użytkownik nie zostanie wybrany przed kliknięciem przycisku, wyświetlony zostanie adekwatny błąd.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk Kamil Wasilewski Dariusz Jóźko
 */
void MainWindow::on_EditUserButton_clicked()
{
    QString Name;
    QString SurName;
    QString genderText;
    gender g;

    int rowidx = ui->AdminUserList->selectionModel()->currentIndex().row();
    if (rowidx < 0) // Nothing was selected.
    {
        QMessageBox::information(this, windowTitle(), tr("Zaznacz użytkownika, którego dane chcesz zedytować i spróbuj ponownie."));
        return;
    }

    Name = ui->AdminUserList->item(rowidx,0)->text();
    SurName = ui->AdminUserList->item(rowidx,1)->text();
    genderText = ui->AdminUserList->item(rowidx,2)->text();
    g = genderText == "M" ? man : woman;

    auw = new AddUserWindow(this,Name,SurName,g);
    if (auw->exec() == 1)
    {
        if (auw->GetGender()==man)
            genderText = "M";
        else
            genderText = "K";
        User::editUser(rowidx,auw->GetName(),auw->GetSurName(),auw->GetGender());
        ui->AdminUserList->setItem(rowidx,0,new QTableWidgetItem(auw->GetName()));
        ui->AdminUserList->setItem(rowidx,1,new QTableWidgetItem(auw->GetSurName()));
        ui->AdminUserList->setItem(rowidx,2,new QTableWidgetItem(genderText));
		auto user = User::GetUser(rowidx);
		userWindow->InsertUserToRanking(user, rowidx);
    }
    delete auw;
}
/**
 * @brief Metoda odpowiedzialna za działanie przycisku "Eksportuj do listy". Uruchamia nowe okno pozwalające wybrać ścieżkę dla pliku CSV, do którego zostanie zapisana lista uczestników.
 * @authors Jarosław Boguta
 */
void MainWindow::on_actionExportToCsv_triggered()
{
	QString filename = QFileDialog::getSaveFileName(
				this, tr("Zapisz plik"), "", tr("Pliki CSV (*.csv);;Wszystkie pliki (*)"));
	if (filename == "")
		return;
	User::exportToCSV(filename);
}
/**
 * @brief Metoda odpowiedzialna za działanie przycisku "Importuj z listy". Uruchamia nowe okno pozwalające wybrać ścieżkę dla pliku CSV, z którego zostanie załadowana lista uczestników.
 * @warning Możliwa jest utrata danych jeśli importowanie z pliku odbędzie się, gdy istnieją użytkownicy w liście.
 * @authors Jarosław Boguta
 */
void MainWindow::on_actionImportFromCsv_triggered()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, windowTitle(), tr("Zaimportowanie nowych danych spowoduje utratę wszystkich niezapisanych danych. Czy kontynuować?"),
                                   QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No)
    {
       return;
    }

	QFileDialog fdialog(this);
	QString filename;
	fdialog.setFileMode(QFileDialog::ExistingFile);
	QStringList filters;
	filters << tr("Plik CSV (*.csv)")
			<< tr("Wszystkie pliki (*)");
	fdialog.setNameFilters(filters);
	if (fdialog.exec())
	{
		filename = fdialog.selectedFiles().front();
	}
	else
		return;

	auto users = User::importFromCSV(filename);
    ui->AdminUserList->clearContents();
    ui->AdminUserList->setRowCount(0);
    userWindow->ClearRanking();

    for (auto user : users)
    {
        ui->AdminUserList->setRowCount(ui->AdminUserList->rowCount() + 1);
        insertUserToList(user);
        if (user->getShoutScore() != 0.0)
            userWindow->InsertUserToRanking(user, ui->AdminUserList->rowCount() - 1);
    }
}

/**
 * @brief Metoda odpowiedzialna za wyświetlanie wyłącznie mężczyzn w oknie przeznaczonym dla publiczności.
 * @param checked Zmienna logiczna umożliwiająca kontrolę wyświetlania.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void MainWindow::on_MenRadioButton_toggled(bool checked)
{
	if (checked == false)
		return;
    userWindow->ShowAll();
    userWindow->SetShowing(m);
    userWindow->HideWomen();
}

/**
 * @brief Metoda odpowiedzialna za wyświetlanie wyłącznie kobiet w oknie przeznaczonym dla publiczności.
 * @param checked Zmienna logiczna umożliwiająca kontrolę wyświetlania.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void MainWindow::on_WomenRadioButton_toggled(bool checked)
{
	if (checked == false)
		return;
    userWindow->ShowAll();
    userWindow->SetShowing(w);
    userWindow->HideMen();
}

/**
 * @brief Metoda odpowiedzialna za wyświetlanie kobiet i mężczyzn w oknie przeznaczonym dla publiczności.
 * @param checked Zmienna logiczna umożliwiająca kontrolę wyświetlania.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void MainWindow::on_AllRadioButton_toggled(bool checked)
{
	if (checked == false)
		return;
    userWindow->ShowAll();
    userWindow->SetShowing(a);
}

/**
 * @brief Metoda odpowiedzialna za uruchomienie procesu kalibracji.
 * @warning Dane kalibracyjne i wszelkie wyniki od nich zależące mogą być błędne, jeśli wybrane urządzenie wejścia nie odbiera sygnału kalibracyjnego.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::on_actionCalibrate_triggered()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("Kalibruj"), tr("Upewnij się, że z obecnie wybranego urządzenia do nagrywania można odebrać sygnał kalibracyjny i kontynuuj."),
								   QMessageBox::Ok|QMessageBox::Cancel);
	if (reply == QMessageBox::Cancel)
	{
	   return;
	}

	calibrator->Calibrate();
	ui->deviceComboBox->setEnabled(false);
	ui->recordButton->setEnabled(false);
}
/**
 * @brief Metoda odpowiedzialna za zamknięcie wszystkich okien po kliknięciu przycisku "Zakończ".
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Kamil Wasilewski
 */
void MainWindow::on_actionClose_triggered()
{
	QApplication::closeAllWindows();
}
/**
 * @brief Metoda odpowiedzialna za uruchomienie procesu kalibracji z pliku.
 * @author Kamil Wasilewski
 */
void MainWindow::on_actionCalibrateFromFile_triggered()
{
    calibrator->CalibrateFromFile("kalibracja.wav");
}

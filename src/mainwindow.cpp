#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audiomodel.h"
#include <QMessageBox>
#include <QFileDialog>
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
    //tworzymy nowe okno mainwindow
    ui->setupUi(this);
	setFixedSize(size());
    //tworzymy okno userWindow
    userWindow = uw;
    //ustawiamy zmienną określającą czy nagrywanie jest w toku na false
    recordOnRun = false;
    //inicjalizujemy kalibrator
	calibrator = new Calibrator(&recorder, this);
    //inicjalizujemy listę dostępnych urządzeń wejścia
    initialiseDeviceList();
    //łączymy przycisk "Nagrywaj" z sygnałem
    connect(ui->recordButton, SIGNAL(pressed()), this, SLOT(proceed()));
    //łączymy combobox wyboru urządzeń z sygnałem
	connect(ui->deviceComboBox, SIGNAL(currentTextChanged(QString)), &recorder, SLOT(InitialiseRecorder(QString)));
    //łączymy przycisk "Kalibracja" z sygnałem
	connect(calibrator, SIGNAL(calibrationStopped()), this, SLOT(onCalibrationStopped()));
    //ustawiamy parametry listy użytkowników w mainWindow
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
    //tworzymy nowe okno po kliknięciu przycisku "X"
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, tr("Zamykanie programu"), tr("Czy chcesz eksportować listę do pliku przed zamknięciem?"), QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);
	if (resBtn == QMessageBox::Cancel)
	{
        //wracamy do programu
		event->ignore();
	}
	else if (resBtn == QMessageBox::Yes)
	{
        //zapisujemy listę do pliku i kończymy program
		on_actionExportToCsv_triggered();
		event->accept();
		userWindow->close();
	}
	else
	{
        //kończymy program
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
    //sprawdzamy czy użytkownik jest zaznaczony
    try
    {    if (rowindex >= 0)
         {
            //sprawdzamy czy użytkownik wykorzystał limit podejść
            if (ui->AdminUserList->item(rowindex,4)->checkState() == Qt::Unchecked)
            {
                //jeżeli wynik jest różny od 0 to znaczy że jedna próba została wykorzystana
                if (ui->AdminUserList->item(rowindex,3)->data(Qt::DisplayRole).toDouble() != 0.0)
                {
                    //zaznaczamy checkboxa
                    ui->AdminUserList->item(rowindex,4)->setCheckState(Qt::Checked);
                }
                //jeżeli nagrywanie jest w trakcie
				if (!recordOnRun)
                {
                    //łączymy recorder z sygnałem
					connect(&recorder, SIGNAL(recordingStopped(const QVector<std::complex<double> > &)), this, SLOT(onRecordingStopped(const QVector<std::complex<double> > &)));
                    currentUser = rowindex; // onRecordingStopped() slot must know, to which user it should assigns shout level.
                    //zaczynamy nagrywanie
                    recorder.Start();
                    //ustalamy zmienną kontrolną na true (nagrywanie trwa)
                    recordOnRun = true;
                    //Zmieniamy napis na przycisku "Nagrywaj"
                    ui->recordButton->setText(tr("Zatrzymaj"));
                    //Uniemożliwiamy użytkownikowi zmianę urządzenia wejścia
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
    // odsyłamy nagranie do metody computeLevel w modelu matematycznym
    double result = AudioModel::computeLevel(complexData, Calibrator::calibrationData);
    //przypisujemy użytkownikowi wynik w dB
	User::setShoutScore(currentUser, result);
    //umieszczamy użytkownika w rankingu
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
    //po zakończeniu kalibracji umożliwiamy użytkonikowi ponowne nagrywanie i zmianę urządzenia wejścia
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
    //zbieramy informacje o dostępnych urządzeniach wejścia
    auto devices = recorder.GetAvailableDevices();
    if (devices.isEmpty())
    {
        //jeżeli ich nie ma
        // Set 'Nagrywaj' button and device list as disabled, so that user could not interact with them.
        ui->deviceComboBox->setEnabled(false);
        ui->recordButton->setEnabled(false);
        QMessageBox::critical(this, windowTitle(), tr("Nie znaleziono żadnych urządzeń do nagrywania. Sprawdź swoje ustawienia i uruchom program ponownie."));
    }
    else
    {
        //jeżeli są, dodajemy je do ComboBoxa
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
    //kolumna 0 = imie
    ui->AdminUserList->setItem(row, 0, new QTableWidgetItem(user->getFirstName()));
    //kolumna 1 = nazwisko
    ui->AdminUserList->setItem(row, 1, new QTableWidgetItem(user->getLastName()));
    QString g = user->getPersonGender() == man ? "M" : "K";
    //kolumna 2 = płeć
    ui->AdminUserList->setItem(row, 2, new QTableWidgetItem(g));
    //kolumna 3 = wynik
    ui->AdminUserList->setItem(row, 3, new QTableWidgetItem(QString::number(user->getShoutScore())));

    auto *checkBoxCell = new QTableWidgetItem(); // Need to assign QTableWidgetItem's address to a pointer in order to call next two functions
    // Do not worry about 'new' operator, QTableWidget can handle this.
    //kolumna 4 = checkbox dot. limitu podejść
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
    //tworzymy nowe okno dodające użytkownika
   auw = new AddUserWindow(this);
   if (auw->exec() == 1) // exec returns 1 when accepted.
   {
       //przypisujemy użytkownikowi dane z okna AddUserWindow
       User U(auw->GetName(), auw->GetSurName(), auw->GetGender(),0);
       //zwiększamy liczbę rzędów w AdminUserList
       ui->AdminUserList->setRowCount(ui->AdminUserList->rowCount()+1);
       //dodajemy użytkownika do listy
       insertUserToList(&U);
   }
   //kasujemy okno AddUserWindow
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
    //ustalamy id rzędu
    int rowidx = ui->AdminUserList->selectionModel()->currentIndex().row();
    if (rowidx < 0) // Nothing was selected.
    {
        QMessageBox::information(this, windowTitle(), tr("Zaznacz użytkownika, którego dane chcesz zedytować i spróbuj ponownie."));
        return;
    }
    //przypisujemy odpowiedniemu rzędowi odpowiednie dane
    Name = ui->AdminUserList->item(rowidx,0)->text();
    SurName = ui->AdminUserList->item(rowidx,1)->text();
    genderText = ui->AdminUserList->item(rowidx,2)->text();
    g = genderText == "M" ? man : woman;
    //nowe okno AddUserWindow do edycji
    auw = new AddUserWindow(this,Name,SurName,g);
    if (auw->exec() == 1)
    {
        if (auw->GetGender()==man)
            genderText = "M";
        else
            genderText = "K";
        //edytujemy użytkownika
        User::editUser(rowidx,auw->GetName(),auw->GetSurName(),auw->GetGender());
        //dodajemy go w odpowiednie miejsca w AdminWindow
        ui->AdminUserList->setItem(rowidx,0,new QTableWidgetItem(auw->GetName()));
        ui->AdminUserList->setItem(rowidx,1,new QTableWidgetItem(auw->GetSurName()));
        ui->AdminUserList->setItem(rowidx,2,new QTableWidgetItem(genderText));
        //do zmiennej user przypisujemy zaznaczonego użytkownika
		auto user = User::GetUser(rowidx);
        //umieszczamy go w userWindow
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
    //tworzymy okno wyboru ścieżki do pliku
	QString filename = QFileDialog::getSaveFileName(
				this, tr("Zapisz plik"), "", tr("Pliki CSV (*.csv);;Wszystkie pliki (*)"));
	if (filename == "")
		return;
    //exportujemy listę do wybranej ścieżki
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
    //sprawdzamy wybór użytkownika po przeczytaniu ostrzeżenia
    reply = QMessageBox::question(this, windowTitle(), tr("Zaimportowanie nowych danych spowoduje utratę wszystkich niezapisanych danych. Czy kontynuować?"),
                                   QMessageBox::Yes|QMessageBox::No);
    //powrót do programu
    if (reply == QMessageBox::No)
    {
       return;
    }
    //tworzymy okno wyboru ścieżki do pliku
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
    //importujemy użytkowników z pliku
	auto users = User::importFromCSV(filename);
    //czyścimy zawartość AdminUserList
    ui->AdminUserList->clearContents();
    //ustawiamy obecny rząd
    ui->AdminUserList->setRowCount(0);
    //czyścimy zawartość userWindow
    userWindow->ClearRanking();
    //dopisujemy każdego użytkownika do obu list
    for (auto user : users)
    {
        //zwiększamy licznik rzędów
        ui->AdminUserList->setRowCount(ui->AdminUserList->rowCount() + 1);
        insertUserToList(user);
        //jeżeli krzyczał, przypisujemy go też do userWindow
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
    //wyswietlamy wszystkich
    userWindow->ShowAll();
     //ustawiamy zmienną kontrolną (m=men)
    userWindow->SetShowing(m);
    //ukrywamy kobiety
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
    //wyswietlamy wszystkich
    userWindow->ShowAll();
    //ustawiamy zmienną kontrolną (w=women)
    userWindow->SetShowing(w);
    //ukrywamy mężczyzn
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
    //wyświetlamy wszystkich
    userWindow->ShowAll();
    //ustawiamy zmienną kontrolną (a=all)
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
    //sprawdzamy wybór użytkownika po wyświetleniu komunikatu
	reply = QMessageBox::question(this, tr("Kalibruj"), tr("Upewnij się, że z obecnie wybranego urządzenia do nagrywania można odebrać sygnał kalibracyjny i kontynuuj."),
								   QMessageBox::Ok|QMessageBox::Cancel);
	if (reply == QMessageBox::Cancel)
	{
	   return;
	}
    //rozpoczynamy kalibracje
	calibrator->Calibrate();
    //uniemożliwiamy wybór urządzeń wejścia
	ui->deviceComboBox->setEnabled(false);
    //uniemozliwiamy naciśnięcie przycisku Nagrywaj
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
    //tworzymy okno wyboru ścieżki do pliku
	QFileDialog fdialog(this);
	QString filename;
    //wyświetlamy dostępne pliki
	fdialog.setFileMode(QFileDialog::ExistingFile);
    //ustalamy filtry
	QStringList filters;
	filters << tr("Plik WAV (*.wav)")
			<< tr("Wszystkie pliki (*)");
	fdialog.setNameFilters(filters);
	if (fdialog.exec())
	{
		filename = fdialog.selectedFiles().front();
	}
	else
		return;
    //wywołujemy kalibrację
	calibrator->CalibrateFromFile(filename);
}

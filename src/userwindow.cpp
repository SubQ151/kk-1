#include "userwindow.h"
#include "ui_userwindow.h"
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTableWidget>
/**
 * @brief Konstruktor. Tworzy okno dostępne dla publiczności.
 * @param parent Okno nadrzędne.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko Kamil Wasilewski
 */
UserWindow::UserWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UserWindow)
{    
    //tworzymy okno UserWindow
    ui->setupUi(this);
    QDesktopWidget *desktop = QApplication::desktop();
    //w kwadracie
    QRect rect = desktop->screenGeometry(1);
    move(rect.topLeft());
    this->showMaximized(); //fullscreen
    //na środku UserList
    setCentralWidget(ui->UserList);
    ui->UserList->setColumnCount(5); //liczba kolumn
    ui->UserList->setColumnHidden(3, true); // Hide ID column.
    ui->UserList->setColumnHidden(4, true); // Hide gender column.
    QStringList Header; //nagłówek
    Header << "Imię" << "Nazwisko" << "Wynik [dB]";
    ui->UserList->setHorizontalHeaderLabels(Header);
    Showing = a; // zmienna kontrolna wyswietlająca wszystkich niezależnie od płci
}
/**
 * @brief Destruktor. Niszczy okno dostępne dla publiczności.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko Kamil Wasilewski
 */
UserWindow::~UserWindow()
{
    delete ui;
}
/**
 * @brief Metoda określająca wielkości kolumn.
 * @param event Zdarzenie.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko Kamil Wasilewski
 */
void UserWindow::resizeEvent(QResizeEvent *event)
{
    ui->UserList->setColumnWidth(0, this->width() * 0.4);
    ui->UserList->setColumnWidth(1, this->width() * 0.4);
    ui->UserList->setColumnWidth(2, this->width() * 0.2);

    QMainWindow::resizeEvent(event);
}
/**
 * @brief Metoda dodająca użytkownika do rankingu.
 * @param user Uczestnik konkursu.
 * @param ID ID uczestnika konkursu.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko Kamil Wasilewski
 */
void UserWindow::InsertUserToRanking(User *user, int ID)
{
    //dla każdego rzędu
     for(int i=0;i<ui->UserList->rowCount();i++)
     {
         //edycja istniejącego użytkownika
         if (ui->UserList->item(i,3)->text() == QString::number(ID))
         {
             ui->UserList->setItem(i,0,new QTableWidgetItem(user->getFirstName())); //dodajemy imie
             ui->UserList->setItem(i,1,new QTableWidgetItem(user->getLastName())); //dodajemy nazwisko
             auto item = new QTableWidgetItem();
             item->setData(Qt::DisplayRole, QVariant(user->getShoutScore())); //dodajemy wynik
             ui->UserList->setItem(i,2,item);
             QString genderText = user->getPersonGender() == man ? "M" : "K";
             ui->UserList->setItem(ui->UserList->rowCount()-1,4,new QTableWidgetItem(genderText)); //dodajemy płeć
             ui->UserList->sortByColumn(2); //sortujemy listę po wyniku
             if(Showing == w) //wyświetlanie kobiet
			 {
				 ShowAll();
				 HideMen();
			 }
             else if(Showing == m) //wyświetlanie mężczyzn
			 {
				 ShowAll();
				 HideWomen();
			 }
             return;
         }
     }
     //dodawanie nowego użytkownika
     ui->UserList->setRowCount(ui->UserList->rowCount()+1); //zwiększamy liczbę rzędów
     ui->UserList->setItem(ui->UserList->rowCount()-1,0,new QTableWidgetItem(user->getFirstName())); //imie
     ui->UserList->setItem(ui->UserList->rowCount()-1,1,new QTableWidgetItem(user->getLastName())); //nazwisko
     auto item = new QTableWidgetItem();
     item->setData(Qt::DisplayRole, QVariant(user->getShoutScore())); // wynik
     ui->UserList->setItem(ui->UserList->rowCount()-1,2,item);
     ui->UserList->setItem(ui->UserList->rowCount()-1,3,new QTableWidgetItem(QString::number(ID))); //ukryte ID
     QString genderText = user->getPersonGender() == man ? "M" : "K";
     ui->UserList->setItem(ui->UserList->rowCount()-1,4,new QTableWidgetItem(genderText)); //płeć
     ui->UserList->sortByColumn(2);
     if(Showing == w)
     {
         ShowAll();
         HideMen();
     }
     else if(Showing == m)
     {
         ShowAll();
         HideWomen();
     }
}
/**
 * @brief Metoda czyszcząca zawartość rankingu.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko Kamil Wasilewski
 */
void UserWindow::ClearRanking()
{
    ui->UserList->clearContents();
    ui->UserList->setRowCount(0); //zmniejszamy liczbę rzędów do 0
}
/**
 * @brief Metoda umożliwiająca kontrolę wyświetlania uczestników według płci przy pomocy zmiennej Showing.
 * @param Showing Zmienna kontrolna.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void UserWindow::SetShowing(showing Showing)
{
    this->Showing=Showing;
}
/**
 * @brief Metoda ukrywająca wszystkich mężczyzn w rankingu.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void UserWindow::HideMen()
{
    for(int i=0;i<ui->UserList->rowCount();i++)
    {
        if(ui->UserList->item(i,4)->text()=="M")
        {
            ui->UserList->setRowHidden(i,true);
        }
    }
}
/**
 * @brief Metoda ukrywająca wszystkie kobiety w rankingu.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void UserWindow::HideWomen()
{
    for(int i=0;i<ui->UserList->rowCount();i++)
    {
        if(ui->UserList->item(i,4)->text()=="K")
        {
            ui->UserList->setRowHidden(i,true);
        }
    }
}
/**
 * @brief Metoda wyświetlająca wszystkich uczestników konkursu w rankingu.
 * @authors Sebastian Zyśk Dariusz Jóźko
 */
void UserWindow::ShowAll()
{
    for(int i=0;i<ui->UserList->rowCount();i++)
    {
        ui->UserList->setRowHidden(i,false);
    }
}






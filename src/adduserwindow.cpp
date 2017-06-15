#include "adduserwindow.h"
#include "ui_adduserwindow.h"
#include <QMessageBox>
/**
 * @brief Konstruktor. Tworzy okno dodawania użytkowników do rankingu.
 * @param parent okno nadrzędne.
 * @authors Marcin Anuszkiewcz Sebastian Zyśk
 */
AddUserWindow::AddUserWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Dodaj użytkownika"));
    setFixedSize(size());
}
/**
 * @brief Konstruktor. Tworzy okno edycji uczestnika konkursu.
 * @param firstName Imię użytkownika.
 * @param lastName Nazwisko użytkownika.
 * @param personGender Płeć użytkownika. Domyślną wartością jest gender::man.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk
 */
AddUserWindow::AddUserWindow(QWidget *parent,QString FirstName, QString SurName, gender g) :
    QDialog(parent),
    ui(new Ui::AddUserWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Edytuj użytkownika"));
    setFixedSize(size());

    ui->FirstNameEdit->setText(FirstName);
    ui->SurNameEdit->setText(SurName);
    if (g == man)
        ui->MenRadioButton->setChecked(true);
    else
        ui->WomenRadioButton->setChecked(true);
}
/**
 * @brief Przycisk "Akceptuj" powodujący zatwierdzenie zmian i dodanie bądź edycję uczestnika konkursu.
 * @warning Jeśli któreś z pól tekstowych, wyświetlany jest adekwatny błąd.
 * @author Marcin Anuszkiewicz Sebastian Zyśk
 */
void AddUserWindow::accept()
{
    if (!(ui->FirstNameEdit->text().isEmpty()))
    {
         name = ui->FirstNameEdit->text();
         if (!(ui->SurNameEdit->text().isEmpty()))
         {
               surname = ui->SurNameEdit->text();
               (ui->MenRadioButton->isChecked()) ? g=man :g=woman;
                QDialog::accept();
         }
         else
         {
            QMessageBox::critical(this, windowTitle(), tr("Pole nazwisko jest puste"));
         }
    }
    else
    {
         QMessageBox::critical(this, windowTitle(), tr("Pole imię jest puste"));
    }
}
/**
 * @brief Destruktor okna dodawania/edycji uczestników konkursu.
 */
AddUserWindow::~AddUserWindow()
{
    delete ui;
}


#ifndef ADDUSERWINDOW_H
#define ADDUSERWINDOW_H

#include <QDialog>
#include "user.h"

namespace Ui {
class AddUserWindow;
}
/**
 * @brief Klasa odpowiadająca za okno dodania lub edycji uczestnika konkursu.
 * @authors Marcin Anuszkiewicz Sebastian Zyśk Dariusz Jóźko
 */
class AddUserWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserWindow(QWidget *parent = 0);
    AddUserWindow(QWidget *parent,QString FirstName, QString SurName, gender g);
    void accept() override;

    /**
     * @brief Getter zwracający imię uczestnika konkursu.
     * @returns name Imie
     * @authors Marcin Anuszkiewicz Sebastian Zyśk
     */
    QString GetName() {return name;}
    /**
     * @brief Getter zwracający nazwisko uczestnika konkursu.
     * @returns surname Nazwisko
     * @authors Marcin Anuszkiewicz Sebastian Zyśk
     */
    QString GetSurName() {return surname;}
    /**
     * @brief Getter zwracający płeć uczestnika konkursu.
     * @returns g Płeć.
     * @authors Marcin Anuszkiewicz Sebastian Zyśk
     */
    gender GetGender() {return g;}
    ~AddUserWindow();

private:
    Ui::AddUserWindow *ui;
    QString name;
    QString surname;
    gender g;
};

#endif // ADDUSERWINDOW_H

#include "calibrator.h"
#include "audiomodel.h"
/**
 *  @param  calibrationData Dane kalibracyjne, przechowujące głośność w decybelach. Początkowo zaincjalizowane na wartość 0.0.
 */
double Calibrator::calibrationData = 0.0;
/**
 *  @brief Kalibrator. Wywołuje pomiar z urządzenia wejścia.
 * @author Pavel Mukha Kamil Wasilewski
 */
Calibrator::Calibrator(Recorder *recorder, QObject *parent) : QObject(parent)
{
	this->recorder = recorder;
}
/**
 *  @brief Metoda wywołująca nagrywanie z urządzenia wejścia a następnie wywołuje metodę recordingStopped. Działa na sygnałach.
 * @authors Pavel Mukha Kamil Wasilewski
 */
void Calibrator::Calibrate()
{
	connect(recorder, SIGNAL(recordingStopped(const QVector<std::complex<double> > &)), this, SLOT(OnRecordingStopped(const QVector<std::complex<double> > &)));
    recorder->Start();
}
/**
 *  @brief Metoda wywołująca kalibrację poprzez pobranie próbki z pliku.
 *  @param  fileName Ścieżka do pliku
 * @authors Kamil Wasilewski
 */
void Calibrator::CalibrateFromFile(const QString &fileName)
{
    connect(recorder, SIGNAL(recordingStopped(const QVector<std::complex<double> > &)), this, SLOT(OnRecordingStopped(const QVector<std::complex<double> > &)));
    recorder->LoadAudioDataFromFile(fileName);
}
/**
 *  @brief Metoda kończąca pobieranie danych kalibracyjnych i odsyłająca je do AudioModel w celu wyliczenia wartości w decybelach.
 *
 *  @param  x tablica liczb zespolonych.
 * @authors Pavel Mukha Kamil Wasilewski
 */
void Calibrator::OnRecordingStopped(const QVector<std::complex<double> > &x)
{
    disconnect(recorder, 0, this, 0);
    calibrationData = AudioModel::computeLevel(x);
    qDebug() << calibrationData;
    calibrationData = 94.0 - calibrationData;
    qDebug() << "Po odjęciu od 94: " << calibrationData;
	emit calibrationStopped();
}

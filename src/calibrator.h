#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <QObject>
#include <QVector>
#include "recorder.h"
/**
 * @brief Klasa odpowiadająca za proces kalibracji programu.
 * @authors Pavel Mukha Kamil Wasilewski
 */
class Calibrator : public QObject
{
    Q_OBJECT
	Recorder *recorder;
public:
	explicit Calibrator(Recorder *recorder, QObject *parent = nullptr);
	void Calibrate();
    void CalibrateFromFile(const QString &fileName);

    static double calibrationData;
signals:
    /**
      * @brief Sygnał kończący kalibrację.
      * @authors Pavel Mukha
      */
	void calibrationStopped();

public slots:
	void OnRecordingStopped(const QVector<std::complex<double> > &x);
};

#endif // CALIBRATOR_H

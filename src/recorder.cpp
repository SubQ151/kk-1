#include "recorder.h"
#include <QDir>
#include <QAudioFormat>
#include <limits>

using std::logic_error;
/**
 * @brief Konstruktor bezparametrowy. Inicjalizuje recorder.
 * @authors Kamil Wasilewski
 */
Recorder::Recorder()
{
    audio = nullptr;
	InitialiseRecorder();
    //tworzymy timer
	setupTimer();
}
/**
 * @brief Destruktor.
 * @authors Kamil Wasilewski
 */
Recorder::~Recorder()
{
    if (audio != nullptr)
        delete audio;
}
/**
 * @brief Metoda przypisująca zmiennej device parametry wybranego urządzenia wejścia.
 * @param deviceName Nazwa urządzenia wejścia
 * @warning Jeśli format wybranego urządzenia nie jest obsługiwany, wybrany zostanie format najlepszego dopasowania.
 * @authors Kamil Wasilewski
 */
void Recorder::InitialiseRecorder(const QString &deviceName)
{
	// destroy old QAudioInput object (if exist)
	if (audio != nullptr)
		delete audio;

	QAudioDeviceInfo device;
    if (deviceName.isEmpty())
		device = QAudioDeviceInfo::defaultInputDevice();
	else
    {
        //ustalamy które z urządzeń zostalo wybrane do nagrywania
		for (auto inputDevice : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
		{
			if (inputDevice.deviceName() == deviceName)
			{
				device = inputDevice;
				break;
			}
		}
	}
    setFormatSettings();
    //sprawdzanie czy format urządzenia jest wspierany, jeśli nie używamy najlepszego dopasowania
	if (!device.isFormatSupported(format))
	{
		qDebug() << "Format not supported, trying to use the nearest.";
		format = device.nearestFormat(format);
	}
    //wyświetlamy ustawienia
	printFormat();
    //przypisujemy zmiennej audio urządzenie do nagrywania i format próbek
    audio = new QAudioInput(device, format);
}
/**
 * @brief Metoda inicjalizująca timer, trwający 5 sekund.
 * @authors Kamil Wasilewski
 */
void Recorder::setupTimer()
{
	timer.setSingleShot(true);
    //ustawiamy interwał na 5s
	timer.setInterval(5000);
    //łączymy timer z sygnałem
	connect(&timer, SIGNAL(timeout()), this, SLOT(Stop()));
}
/**
 * @brief Metoda ustawiająca format próbek.
 * @authors Kamil Wasilewski
 */
void Recorder::setFormatSettings()
{
    format.setChannelCount(1); //1 kanał
    format.setSampleRate(48000); //48kHz częstotliwości
    format.setCodec("audio/pcm"); //kodek pcm
    format.setSampleSize(16); //wielkość próbek
    format.setByteOrder(QAudioFormat::LittleEndian); //kolejność bitów
    format.setSampleType(QAudioFormat::SignedInt); //typ probek
}
/**
 * @brief Metoda rozpoczynająca proces pobierania dźwięków z urządzenia wejścia do bufora.
 * @authors Kamil Wasilewski
 */
void Recorder::Start()
{
    buffer.buffer().clear(); // Flush data from underlying QByteArray internal buffer.
    //otwieramy buffer i rozpoczynamy nagrywanie
    buffer.open(QIODevice::ReadWrite);
    audio->start(&buffer);

	// Record 5 seconds.
    timer.start();
}
/**
 * @brief Metoda kończąca przechwytywanie danych do buforu.
 * @authors Kamil Wasilewski
 */
void Recorder::Stop()
{
    timer.stop(); // Stop a timer in case user aborts recording.
    //kończymy nagrywanie i zamykamy buffer
    audio->stop();
	buffer.close();
    //parsujemy zawartość buforu
	parseBufferContent(buffer.data());
    //wysyłamy sygnał do metody recordingStopped
	emit recordingStopped(complexData);
}
/**
 * @brief Metoda wyświetlająca format pobieranych danych.
 * @authors Kamil Wasilewski
 */
void Recorder::printFormat() const
{
	qDebug() << "Channel count: " << format.channelCount();
	qDebug() << "Byte order: " << format.byteOrder();
	qDebug() << "Sample type: " << format.sampleType();
	qDebug() << "Sample size: " << format.sampleSize();
	qDebug() << "Sample rate: " << format.sampleRate();
    qDebug() << "";
}
/**
 * @brief Metoda zwracająca listę dostępnych urządzeń wejścia.
 * @return devicesNames lista nazw dostępnych urządzeń wejścia.
 * @authors Kamil Wasilewski
 */
QStringList Recorder::GetAvailableDevices() const
{
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	QStringList devicesNames;
    for (auto device : devices)
    {
		devicesNames.append(device.deviceName());
    }
	return devicesNames;
}
/**
 * @brief Metoda parsująca dane zawarte w buforze.
 * @authors Kamil Wasilewski
 */
void Recorder::parseBufferContent(const QByteArray &data)
{
	QDataStream stream(data);
	parse(stream);
}
/**
 * @brief Metoda wczytująca dane Audio z pliku.
 * @param fileName Nazwa pliku.
 * @authors Kamil Wasilewski
 */
void Recorder::LoadAudioDataFromFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QFile::ReadOnly);
	file.seek(44); // Skip WAV header.
    QDataStream fstream(&file);
	parse(fstream);
    file.close();
    emit recordingStopped(complexData);
}
/**
 * @brief Metoda parsująca dane.
 * @authors Kamil Wasilewski
 */
void Recorder::parse(QDataStream &stream)
{
	complexData.clear();
    stream.setByteOrder(QDataStream::LittleEndian); //ustawaimy kolejność bitów
	while (!stream.atEnd())
	{
		short i;
		stream >> i;
		double j = (double) i / (double) std::numeric_limits<short>::max(); // Scale to [-1,1] range.
        complexData.push_back(std::complex<double>(j, 0.0)); //wszelkie zera wysyłamy na koniec danej struktury
	}
}

#define _USE_MATH_DEFINES

#include "audiomodel.h"
#include <fftw3.h>
#include <cmath>
/**
 *  @brief Metoda korzystająca z szybkiej transformaty Fourier'a, która zwraca FFT tablicy liczb zespolonych.
 *  @param  x tablica liczb zespolonych
 *  @return Zwraca FFT tablicy liczb zespolonych <tt>x</tt>
 * @authors Adrian Borucki Magdalena Buczyńska Adrianna Łuczak Kamil Wasilewski
 */
QVector<complex<double> > AudioModel::fft(const QVector<complex<double> > &x)
{
    int N = x.length();

    fftw_complex *in, *out;
    fftw_plan p;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; i++) {
        in[i][0] = x[i].real();
        in[i][1] = x[i].imag();
    }

    fftw_execute(p);

    auto y = QVector<complex<double>>(N);
    for (int i = 0; i < N; i++) {
        y[i] = complex<double>(out[i][0], out[i][1]);
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
	return y;
}
/**
 *  @brief Metoda obliczająca charakterystykę A
 *  @param  frequency częstotliwość
 *  @return zwraca licznik podzielony przez mianownik
 * @authors Kamil Wasilewski
 */
double AudioModel::filterA(double frequency)
{
	const double c1 = std::pow(12194.217, 2);
	const double c2 = std::pow(20.598997, 2);
	const double c3 = std::pow(107.65265, 2);
	const double c4 = std::pow(737.86223, 2);
    //kwadrat częstotliwości
	double f = frequency * frequency;
	double numerator = f * f * c1; // 'licznik'
	double denominator = (f + c2) * std::sqrt((f + c3) * (f + c4)) * (f + c1); // 'mianownik'
	return numerator / denominator;
}
/**
 *  @brief Metoda obliczająca charakterystykę mikrofonu i głośność w decybelach orginalnego sygnału z urządzenia wejścia przy pomocy twierdzenia Parsevala.
 *  @param x Orginalny sygnał z urządzenia wejścia
 *  @param calibrationData Dane kalibracyjne
 *  @return Głośność w decybelach obliczona przy pomocy twierdzenia Parsevala.
 *  @authors Kamil Wasilewski Dariusz Jóźko
 */
double AudioModel::computeLevel(const QVector<std::complex<double> > &x, double calibrationData)
{
	int samples = x.length(); // Number of samples (f * seconds)
	double total_p = 0.0;

	const long long y = f * (long long) samples;
    //fft
	auto xdft = fft(x);
    //w pętli liczymy moduł każdej liczby zespolonej po fft
	for (int i = 0; i < samples / 2 + 1; ++i)
	{
		double p = std::abs(xdft[i]);
		if (i != 0 && i != samples / 2)
			p *= filterA(i);
		p = std::pow(p, 2) / y;
		if (i != 0 && i != samples / 2)
			p *= 2;
		total_p += p;
	}
    //zwracamy wartość w dB
	return 10 * log10(total_p) + calibrationData;
}

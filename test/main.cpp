//
// Created by michel on 10-9-16.
//

#define PEAK_DETECTION_LOGGING 2

#include <jack/jack.h>
#include <iostream>
#include <cstdio>
#include <tdap/Delay.hpp>
#include <tdap/PerceptiveRms.hpp>
#include <tdap/PeakDetection.hpp>
#include <tdap/TrueFloatingPointWindowAverage.hpp>


using namespace tdap;
using namespace std;

static void testTrueAverage()
{
    const size_t maxWindowSize = 100;
    const size_t errorTimeConstant = maxWindowSize * 100;
    const double relativeErrorNoise = 1e-6;
    const double amplitude = 1.0;
    const size_t largeWindow = 100;
    const size_t smallWindow = 10;
    const size_t printInterval = 1;

    TrueFloatingPointWeightedMovingAverage<double> smallAverage(
            maxWindowSize, errorTimeConstant);
    TrueFloatingPointWeightedMovingAverage<double> largeAverage(
            maxWindowSize, errorTimeConstant);

    TrueFloatingPointWeightedMovingAverageSet<double> set(
            maxWindowSize, errorTimeConstant, 2, 0.0);

    smallAverage.setAverage(0);
    largeAverage.setAverage(0);
    set.setAverages(0);

    printf("\nSetting small average:\n");
    smallAverage.setWindowSize(smallWindow);
    printf("\nSetting large average:\n");
    largeAverage.setWindowSize(largeWindow);
    printf("\nSetting set, small average:\n");
    set.setWindowSizeAndScale(0, smallWindow, 1.0);
    printf("\nSetting set, large average:\n");
    set.setWindowSizeAndScale(1, largeWindow, 2.0);

    printf("Start....\n");
    for (size_t i = 0; i < largeWindow * 5; i++) {
        const double input = i > largeWindow && i <= 2*largeWindow ? amplitude : 0.0;
        smallAverage.addInput(input);
        largeAverage.addInput(input);
        double setAvg = set.addInputGetMax(input, 0.0);
        if (i % printInterval == 0) {
            printf("[%5zu] input=%8.3lf ; avg1=%18.16lf ; avg2=%18.16lf ; set1=%18.16lf ; set2=%18.16lf ; setMax=%18.16lf\n",
                    i,
                    input,
                    smallAverage.getAverage(), largeAverage.getAverage(),
                    set.getAverage(0), set.getAverage(1),
                    setAvg);
        }
    }
}


template <typename S>
static void testPeakDetector()
{
    static constexpr size_t RANGE = 100;
    static constexpr size_t THRESHOLD = 25;
    static constexpr size_t WINDOW = 288;
    static constexpr size_t RUNLENGTH = WINDOW * 100;
    static constexpr size_t PERIODS = 5;
    static constexpr size_t RANDOM_PEAK = 17 * WINDOW / 10;

    PeakMemory<S> memory(288);
    PeakDetector<S> detector(288, 0.7, 0.3, 1);

    S input[RUNLENGTH];
    char line[RANGE + 1];

    for (size_t i = 0; i < RUNLENGTH; i++) {
        input[i] = THRESHOLD * (1.0 + sin(M_2_PI * i / PERIODS) * rand() / RAND_MAX);
        if (i % RANDOM_PEAK == 0) {
            input[i] += 50;
        }
    }


    size_t samples = memory.setSamples(WINDOW);
    static constexpr const char * DIGIT = " iP*";

    for (size_t i = 0; i < RUNLENGTH; i++) {
        double in = input[i];
        double peak = memory.addSampleGetPeak(in);
        double delayed = (i >= samples) ? input[i - samples] : 0.0;
        double fault = delayed / peak;
        if (fault > 1 || (i % WINDOW == 0)) {
            for (size_t at = 0; at < RANGE; at++) {
                int dig = 0;
                if (at == static_cast<size_t>(delayed)) {
                    dig |= 1;
                }
                if (at == static_cast<size_t>(peak)) {
                    dig |= 2;
                }
                line[at] = DIGIT[dig];
            }
            line[RANGE] = '\0';
            if (fault > 0) {
                printf("[%5zu]\t[%s] %6.04lf\n", i, line, fault);
            }
            else {
                printf("[%5zu]\t[%s] %7s\n", i, line, "");
            }
        }
    }

    memory.setSamples(WINDOW);
    samples = detector.setSamplesAndThreshold(WINDOW, THRESHOLD);

    printf("\nUsing LIMITER\n\n");
    static constexpr const char * DIGIT_LIMIT = " io*dddd";
    double maxFault = 0;
    for (size_t i = 0; i < RUNLENGTH; i++) {
        double detect = detector.addSampleGetDetection(input[i]);
        double in = i >= samples ? input[i - samples] : 0.0;
        double gain = static_cast<double>(THRESHOLD) / detect;
        double out = in * gain;
        double fault = in / detect;
        maxFault = Floats::max(maxFault, fault);
        if (fault > 1 || (i % WINDOW == 0)) {
            for (size_t at = 0; at < RANGE; at++) {
                int dig = 0;
                if (at == static_cast<size_t>(in)) {
                    dig |= 1;
                }
                if (at == static_cast<size_t>(out)) {
                    dig |= 2;
                }
                if (at == static_cast<size_t>(detect)) {
                    dig |= 4;
                }
                line[at] = DIGIT_LIMIT[dig];
            }
            line[RANGE] = '\0';
            if (fault > 1) {
                printf("[%5zu][%s] %6.04lf! %6.02lf in  %6.02lf out   %6.04lf gain   %6.02lf detect\n",
                       i, line, fault, in, out, gain, detect);
            }
            else {
                printf("[%5zu][%s] %7s %6.02lf in  %6.02lf out   %6.04lf gain   %6.02lf detect\n",
                       i, line, "", in, out, gain, detect);

            }
        }
    }
    printf("Maximum fault: %lf\n", maxFault);
}


static void printDelayEntry(const typename MultiChannelAndTimeDelay<int>::Entry &entry)
{
	cout << "\tdelay=" << entry.delay_ << "; end=" << entry.end_ << "; write=" << entry.write_ << "; read=" << entry.read_ << endl;
}


static void printDelayState(const MultiChannelAndTimeDelay<int> &delay)
{
	cout << "Delay channels=" << delay.channels_ << "; max-channels=" << delay.maxChannels_ << endl;
	for (size_t i = 0, t = 0; t <= delay.maxDelay_; t++)  {
		cout << "\t[" << t << "]";
		for (size_t channel = 0; channel < delay.channels_; channel++, i++) {
			cout << " " << delay.buffer_[i];
		}
		cout << endl;
	}
	for (size_t channel = 0; channel < delay.channels_; channel++) {
		printDelayEntry(delay.entry_[channel]);
	}
};

void testMultiTimeDelay()
{
	MultiChannelAndTimeDelay<int> delay(4, 4);
	delay.setChannels(4);
	printDelayState(delay);
	delay.setDelay(0, 0);
	printDelayState(delay);
	delay.setDelay(1, 1);
	printDelayState(delay);
	delay.setDelay(2, 2);
	printDelayState(delay);
	delay.setDelay(3, 3);

	for (int i = 0; i < 100; i++) {
		cout << "channel 0: getAndSet(" << (i + 100) << ") = " << delay.setAndGet(0, i + 100) << endl;
		cout << "channel 1: getAndSet(" << (i + 200) << ") = " << delay.setAndGet(1, i + 200) << endl;
		cout << "channel 2: getAndSet(" << (i + 300) << ") = " << delay.setAndGet(2, i + 300) << endl;
		cout << "channel 3: getAndSet(" << (i + 400) << ") = " << delay.setAndGet(3, i + 400) << endl;
		printDelayState(delay);
		delay.next();
	}
}

int main(int c, const char *args[])
{
//	testMultiTimeDelay();
//	testTrueAverage();
        testPeakDetector<double>();
	return 0;
}
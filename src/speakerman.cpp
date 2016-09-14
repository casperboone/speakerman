/*
 * Speakerman.cpp
 *
 * Part of 'Speaker management system'
 *
 * Copyright (C) 2013 Michel Fleur.
 * https://github.com/emmef/simpledsp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <thread>

#include <speakerman/jack/JackClient.hpp>
#include <speakerman/jack/SignalHandler.hpp>
#include <speakerman/SpeakermanConfig.hpp>
#include <speakerman/SpeakerManager.hpp>
#include <speakerman/SpeakermanWebServer.hpp>
#include <malloc.h>
#include <sys/mman.h>

using namespace speakerman;
using namespace tdap;

typedef double sample_t;
typedef double accurate_t;

template<class T>
class Owner
{
	atomic<T *> __client;

public:
	Owner() {
		__client.store(0);
	}

	void set(T * client)
	{
		T* previous = __client.exchange(client);
		if (previous != nullptr) {
			cout << "Delete client" << endl;

			delete previous;
		}
	}

	T &get() const
	{
		return *__client.load();
	}

	~Owner()
	{
		set(nullptr);
	}
};

Owner<AbstractSpeakerManager> manager;
SpeakermanConfig configFileConfig;
static volatile int userInput;

inline static accurate_t frequencyWeight(accurate_t f, accurate_t shelve1, accurate_t shelve2, accurate_t power)
{
		accurate_t fRel = pow(f / shelve1, power);
		accurate_t fShelve2Corr = pow(1.0 / shelve2, power);
		return (1 + fRel * fShelve2Corr) / (1.0 + fRel);
}

static void webServer()
{
	web_server server(manager.get());

	try {
		server.open("8088", 60, 10, nullptr);
		server.work(nullptr);
	}
	catch (const std::exception &e) {
		std::cerr << "Web server error: " << e.what() << std::endl;
	}
	catch (const signal_exception &e) {
		cerr << "Web server thread stopped" << endl;
		e.handle();
	}
}

int mainLoop(Owner<JackClient> &owner)
{
	std::thread webServerThread(webServer);
	webServerThread.detach();

	const std::chrono::milliseconds duration(100);
	try {
		while (true) {
			this_thread::sleep_for(duration);
			SignalHandler::check_raised();
		}
		cout << "Bye!";
		return 0;
	}
	catch (const signal_exception &e) {
		e.handle();
		return e.signal();
	}
}


//speakerman::config::Reader configReader;
speakerman::server_socket webserver;

template<typename F, size_t GROUPS, size_t CROSSOVERS>
AbstractSpeakerManager *createManagerSampleType(const SpeakermanConfig & config)
{
	static_assert(is_floating_point<F>::value, "Sample type must be floating point");

	switch (config.groupChannels) {
	case 1:
		return new SpeakerManager<F, 1, GROUPS, CROSSOVERS>(config);
	case 2:
		return new SpeakerManager<F, 2, GROUPS, CROSSOVERS>(config);
	case 3:
		return new SpeakerManager<F, 3, GROUPS, CROSSOVERS>(config);
	case 4:
		return new SpeakerManager<F, 4, GROUPS, CROSSOVERS>(config);
	case 5:
		return new SpeakerManager<F, 5, GROUPS, CROSSOVERS>(config);
	}
	throw invalid_argument("Number of channels per group must be between 1 and 5");
}

template <typename F, size_t CROSSOVERS>
static AbstractSpeakerManager *createManagerGroup(const SpeakermanConfig & config)
{

	switch (config.groups) {
	case 1:
		return createManagerSampleType<F, 1, CROSSOVERS>(config);
	case 2:
		return createManagerSampleType<F, 2, CROSSOVERS>(config);
	case 3:
		return createManagerSampleType<F, 3, CROSSOVERS>(config);
	case 4:
		return createManagerSampleType<F, 4, CROSSOVERS>(config);
	}
	throw invalid_argument("Number of groups must be between 1 and 4");
}

template <typename F>
static AbstractSpeakerManager *createManager(const SpeakermanConfig & config)
{

	switch (config.crossovers) {
	case 1:
		return createManagerGroup<F, 1>(config);
	case 2:
		return createManagerGroup<F, 2>(config);
	case 3:
		return createManagerGroup<F, 3>(config);
	}
	throw invalid_argument("Number of crossovers must be between 1 and 3");
}

using namespace std;
int main(int count, char * arguments[]) {
	configFileConfig = readSpeakermanConfig(true);

	cout << "Executing " << arguments[0] << endl;


	Owner<JackClient> clientOwner;
	auto result = JackClient::createDefault("Speaker manager");
	clientOwner.set(result.getClient());
	manager.set(createManager<double>(configFileConfig));
	mallopt(M_TRIM_THRESHOLD, -1);
	mallopt(M_MMAP_MAX, 0);

	const char * all = ".*";
	PortNames inputs = clientOwner.get().portNames(all, all, JackPortIsPhysical|JackPortIsOutput);
	PortNames outputs = clientOwner.get().portNames(all, all, JackPortIsPhysical|JackPortIsOutput);

	if (!clientOwner.get().setProcessor(manager.get())) {
		std::cerr << "Failed to set processor" << std::endl;
		return 1;
	}

	std::cout << "activate..." << std::endl;
	clientOwner.get().setActive();

	std::cout << "activated..." << std::endl;
	return mainLoop(clientOwner);
}


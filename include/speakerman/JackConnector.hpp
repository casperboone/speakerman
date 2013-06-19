/*
 * JackConnector.hpp
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

#ifndef SMS_SPEAKERMAN_JACKCONNECTOR_GUARD_H_
#define SMS_SPEAKERMAN_JACKCONNECTOR_GUARD_H_

#include <string>
#include <atomic>
#include <jack/jack.h>
#include <simpledsp/List.hpp>
#include <speakerman/utils/Mutex.hpp>

namespace speakerman {

using namespace simpledsp;

enum class Direction
{
	IN, OUT
};

class JackClient;
class JackPort
{
	std::string name;
	Direction direction;
	jack_port_t * port = nullptr;

	void registerPort(jack_client_t *client);
	void deRegisterPort();
	jack_default_audio_sample_t * getBuffer(jack_nframes_t frames);

public:
	JackPort(std::string name, Direction direction);
	~JackPort();
	friend class JackProcessor;
};


enum class ClientState
{
	INITIAL,
	CLOSED,
	DEFINED_PORTS,
	REGISTERED,
	ACTIVE
};

class JackProcessor
{
	friend class JackClient;

	List<JackPort> inputs;
	List<JackPort> outputs;

	void registerPorts(jack_client_t *client);
	void unRegisterPorts();

protected:

	JackProcessor();
	void addInput(string name);
	void addOutput(string name);
	const jack_default_audio_sample_t *getInput(size_t number, jack_nframes_t frameCount) const;
	jack_default_audio_sample_t *getOutput(size_t number, jack_nframes_t frameCount) const;

public:

	virtual void prepareActivate() = 0;
	virtual void prepareDeactivate() = 0;

	virtual bool process(jack_nframes_t frameCount) = 0;
	virtual bool setSampleRate(jack_nframes_t sampleRate) = 0;
	virtual void shutdownByServer() = 0;

	virtual ~JackProcessor() {};
};

template <typename T> class LockFreeAtomicState {
public:
	LockFreeAtomicState(T initialValue);
	virtual bool set(T newValue);
	virtual bool get(T newValue);
	virtual bool compareAndSet(T expected, T newValue) = 0;
	virtual ~	LockFreeAtomicState() {};
};


class JackClient
{
	speakerman::Mutex m;
	string name;
	jack_client_t *client = nullptr;
	ClientState state = ClientState::INITIAL;
	JackProcessor &processor;

	static int rawProcess(jack_nframes_t nframes, void* arg);
	static void rawShutdown(void* arg);
	static int rawSetSampleRate(jack_nframes_t nframes, void* arg);

	void checkCanAddIO();
	void shutdownByServer();
	void unsafeOpen();

protected:

public:
	JackClient(string name, JackProcessor &processor);
	void open();
	void activate();
	void deactivate();
	void close();

	virtual ~JackClient();
};

} /* End of namespace speakerman */

#endif /* SMS_SPEAKERMAN_JACKCONNECTOR_GUARD_H_ */
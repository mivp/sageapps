#pragma once

#include "DeckLinkAPI_h.h"
#include "DeckLinkDevice.h"
#include <iostream>

class DeckLinkDevice;
class DeckLinkDeviceDiscovery;


class Callback {
public:
	virtual ~Callback() { std::cout << "C Callback::~Callback()" << std::endl; }
	virtual void run(void* msg, int length) = 0;
};


class DeckLinkManager {
private:
	//AncillaryDataStruct		m_ancillaryData;
	//CCriticalSection			m_critSec; // to synchronise access to the above structure
	DeckLinkDevice*				m_selectedDevice;
	DeckLinkDeviceDiscovery*	m_deckLinkDiscovery;

	Callback*					m_callback;

public:
	DeckLinkManager();
	~DeckLinkManager();

	void delCallback();
	void setCallback(Callback* cb);

	void AddDevice(IDeckLink* deckLink);
	void StartCapture();
	void StopCapture();
	void FrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame);
};
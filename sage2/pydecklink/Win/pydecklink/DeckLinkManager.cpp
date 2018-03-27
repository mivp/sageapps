#include "DeckLinkManager.h"
#include <iostream>
//#include <thread>
#include <Python.h>

using namespace std;

DeckLinkManager::DeckLinkManager(): m_deckLinkDiscovery(0), m_selectedDevice(0), m_callback(0)
{
	cout << "DeckLinkManager" << endl;
	m_deckLinkDiscovery = new DeckLinkDeviceDiscovery(this);
	if (!m_deckLinkDiscovery->Enable())
		cout << "Please install the Blackmagic Desktop Video drivers to use the features of this application." << endl;
}

DeckLinkManager::~DeckLinkManager()
{
	delCallback();
	if (m_selectedDevice)
		delete m_selectedDevice;
	if (m_deckLinkDiscovery)
		delete m_deckLinkDiscovery;
}

void DeckLinkManager::delCallback() {
	if (m_callback) {
		delete m_callback;
		m_callback = 0;
	}
}

void DeckLinkManager::setCallback(Callback* cb) {
	delCallback();
	m_callback = cb;
	//cout << "setCallback " << std::this_thread::get_id() << " m_callback: " << m_callback << endl;
}

void DeckLinkManager::AddDevice(IDeckLink* deckLink) {
	cout << "Deviced added" << endl;
	m_selectedDevice = new DeckLinkDevice(this, deckLink);

	// Initialise new DeckLinkDevice object
	if (!m_selectedDevice->Init())
	{
		m_selectedDevice->Release();
		return;
	}
	wcout << (const wchar_t*)m_selectedDevice->GetDeviceName() << endl;

	std::vector<CString> modeNames;
	m_selectedDevice->GetDisplayModeNames(modeNames);
	for (int i = 0; i < modeNames.size(); i++)
		wcout << (const wchar_t*)modeNames[i] << endl;

	StartCapture();
	//cout << "AddDevice "  << std::this_thread::get_id() << " m_callback: " << m_callback << endl;
}

void DeckLinkManager::StartCapture() {
	if (!m_selectedDevice)
		return;
	cout << "Start capture" << endl;
	m_selectedDevice->StartCapture(9, NULL, false);
}

void DeckLinkManager::StopCapture() {
	if (!m_selectedDevice)
		return;
	cout << "Stop capture" <<  endl;
	m_selectedDevice->StopCapture();
}

void DeckLinkManager::FrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame) {
	
	if (videoFrame) {
		cout << "Frame arrived" << endl;
	}
	
	if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
	{
		fprintf(stderr, "Frame received - No input signal detected\n");
		return;
	}
	//cout << "FrameArrived " << std::this_thread::get_id() << " m_callback: " << m_callback << endl;
	if (m_callback) {
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();

		m_callback->run("hello\0 from C", 13);

		PyGILState_Release(gstate);
	}
}
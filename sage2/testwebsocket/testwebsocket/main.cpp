#include "websocketio.h"
#include <iostream>

WebSocketIO* wsio;
std::string uniqueID;
std::string ws_host;
std::string applicationName;

void ws_open(WebSocketIO*);
void ws_initialize(WebSocketIO*, boost::property_tree::ptree);
void ws_requestNextFrame(WebSocketIO*, boost::property_tree::ptree);
void ws_stopMediaCapture(WebSocketIO*, boost::property_tree::ptree);
void ws_setItemPositionAndSize(WebSocketIO*, boost::property_tree::ptree);
void ws_finishedResize(WebSocketIO*, boost::property_tree::ptree);
void ws_setupDisplayConfiguration(WebSocketIO*, boost::property_tree::ptree);

using namespace std;

#define WIDTH  1920
#define HEIGHT 1080
#define BPP    3
#define IMG_SIZE (WIDTH*HEIGHT*BPP)

bool request_frame = false;
unsigned char *data_white;
unsigned char *data_black;
unsigned long framecount = 0;

int main(int argc, char* argv[]) {

	string address = "ws://localhost:9292";

	data_black = (unsigned char*)malloc(IMG_SIZE);
	memset(data_black, 0, IMG_SIZE);

	data_white = (unsigned char*)malloc(IMG_SIZE);
	memset(data_white, 255, IMG_SIZE);

	wsio = new WebSocketIO(address, NULL);
	wsio->open(ws_open);

	getchar();

	free(data_white);

	return 0;
}


void ws_open(WebSocketIO* ws) {
	
	ws->on("initialize", ws_initialize);
	ws->on("requestNextFrame", ws_requestNextFrame);
	// ws->on("stopMediaBlockStream", ws_stopMediaCapture);
	//ws->on("stopMediaCapture", ws_stopMediaCapture);
	//ws->on("setupDisplayConfiguration", ws_setupDisplayConfiguration);
	
	boost::property_tree::ptree data;
	data.put<std::string>("clientType", "testapp");
	boost::property_tree::ptree req;
	req.put<bool>("config", true);
	req.put<bool>("version", false);
	req.put<bool>("time", false);
	req.put<bool>("console", false);
	data.put_child("requests", req);

	ws->emit("addClient", data);
}

void ws_setupDisplayConfiguration(WebSocketIO*, boost::property_tree::ptree data) {
	std::string dname = data.get<std::string>("name");
	int dwidth = data.get<int>("totalWidth");
	int dheight = data.get<int>("totalHeight");
	fprintf(stderr, "Display> %s %d %d\n", dname.c_str(), dwidth, dheight);
}

void ws_initialize(WebSocketIO* ws, boost::property_tree::ptree data) {

	fprintf(stderr, "Initializing connection\n");

	uniqueID = data.get<std::string>("UID");

	boost::property_tree::ptree emit_data;
	emit_data.put<std::string>("id", uniqueID + "|0");
	emit_data.put<std::string>("title", "testapp");
	emit_data.put<std::string>("color", "#c1cdcd");
	emit_data.put<std::string>("colorspace", "RGB");
	emit_data.put<int>("width", WIDTH);
	emit_data.put<int>("height", HEIGHT);
	ws->emit("startNewMediaBlockStream", emit_data);
	fprintf(stderr, "Start streaming\n");
}


void sendFrame(WebSocketIO* ws) {
	
	framecount++;
	request_frame = false;
	std::string data = uniqueID + "|0" + '\0';
	cout << "convert data start" << endl;
	string tmp;
	if (framecount % 2)
		tmp = std::string( reinterpret_cast<char*>(data_white), IMG_SIZE);
	else
		tmp = std::string( reinterpret_cast<char*>(data_black), IMG_SIZE);

	cout << "convert data end, now add data" << endl;
	data += tmp;
	cout << "add data end" << endl;

	int length = uniqueID.length() + 3 + IMG_SIZE;
	ws->emit_binary("updateMediaBlockStreamFrame", (unsigned char*)data.c_str(), length);

	cout << "sendFrame " << framecount << " with length " << length << endl;
}

void ws_requestNextFrame(WebSocketIO* ws, boost::property_tree::ptree data) {
	//sail *sageInf = (sail*)ws->getPointer();
	fprintf(stderr, "ws_requestNextFrame\n");
	request_frame = true;
	sendFrame(ws);
	/*
	sageInf->got_request = true;
	if (sageInf->got_frame) {
		swapBuffer(sageInf);
	}
	*/
}
/*
void ws_stopMediaCapture(WebSocketIO* ws, boost::property_tree::ptree data) {
	std::string streamId = data.get<std::string>("streamId");
	int idx = atoi(streamId.c_str());
	fprintf(stderr, "STOP MEDIA CAPTURE: %d\n", idx);

	sail *sageInf = (sail*)ws->getPointer();
	deleteSAIL(sageInf);
}

void ws_setItemPositionAndSize(WebSocketIO* ws, boost::property_tree::ptree data) {
	sail *sageInf = (sail*)ws->getPointer();

	// update browser window size during resize
	if (continuous_resize) {
		std::string id = data.get<std::string>("elemId");
		std::vector<std::string> elemData = split(id, '|');
		std::string uid = "";
		int idx = -1;
		if (elemData.size() == 2) {
			uid = elemData[0];
			idx = atoi(elemData[1].c_str());
		}

		if (uid == sageInf->uniqueID) {
			std::string w = data.get<std::string>("elemWidth");
			std::string h = data.get<std::string>("elemHeight");
			std::string x = data.get<std::string>("elemLeft");
			std::string y = data.get<std::string>("elemTop");
			float neww = atof(w.c_str());
			float newh = atof(h.c_str());
			float newx = atof(x.c_str());
			float newy = atof(y.c_str());
			printf("New pos & size: %.0f x %.0f - %.0f x %.0f\n", newx, newy, neww, newh);
		}
	}
}

void ws_finishedResize(WebSocketIO* ws, boost::property_tree::ptree data) {
	sail *sageInf = (sail*)ws->getPointer();

	std::string id = data.get<std::string>("id");
	std::vector<std::string> elemData = split(id, '|');
	std::string uid = "";
	int idx = -1;
	if (elemData.size() == 2) {
		uid = elemData[0];
		idx = atoi(elemData[1].c_str());
	}

	if (uid == sageInf->uniqueID) {
		std::string w = data.get<std::string>("elemWidth");
		std::string h = data.get<std::string>("elemHeight");

		float neww = atof(w.c_str());
		float newh = atof(h.c_str());

		fprintf(stderr, "New size: %.0f x %.0f\n", neww, newh);
	}
}

*/
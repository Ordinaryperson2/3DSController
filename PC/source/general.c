#include "wireless.h"

#include "general.h"

void error(const char *functionName) {
	char errorMsg[92];
	ZeroMemory(errorMsg, 92);

	sprintf(errorMsg, "Call to %s returned error %d!", (char *)functionName, WSAGetLastError());

	MessageBox(NULL, errorMsg, "socketIndication", MB_OK);

	closesocket(client);
	closesocket(listener);
	WSACleanup();

	exit(0);
}

int clamp	(int val, int min, int max) {
	if (val < min) {
		return min;
	}

	if (val > max) {
		return max;
	}

	return val;
}

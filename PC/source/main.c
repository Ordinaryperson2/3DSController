// 3DS Controller Server

#define VERSION 0.4

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "wireless.h"
#include "keys.h"
#include "general.h"
#include "joystick.h"
#include "settings.h"
#include "keyboard.h"
#include "screenshot.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int nShow) {
	printf("3DS Controller Server %.1f\n", VERSION);
	
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN);
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	double widthMultiplier = screenWidth / 320.0;
	double heightMultiplier = screenHeight / 240.0;
	
	screenshot(SCREENSHOT_NAMEL, TRUE, 0, 0, 18);
	
	bool vJoy = true;
	UINT iInterface = 1;
	
	if(vJoy && !vJoyEnabled()) {
		printf("vJoy failed (1)! Buttons will still work, but joy stick won't work.\n");
		vJoy = false;
	}
	
	enum VjdStat status = GetVJDStatus(iInterface);
	if(vJoy && (status == VJD_STAT_OWN || (status == VJD_STAT_FREE && !AcquireVJD(iInterface)))) {
		printf("vJoy failed (2)! Buttons will still work, but joy stick won't work.\n");
		vJoy = false;
	}
	
	ContPovNumber = GetVJDContPovNumber(iInterface);
	//int DiscPovNumber = GetVJDDiscPovNumber(iInterface);
	
	if(vJoy && !updateJoystick(128 * 128, 128 * 128)) {
		printf("vJoy failed (3)! Buttons will still work, but joy stick won't work.\n");
		vJoy = false;
	}
	
	if(!readSettings()) {
		printf("Couldn't read settings file, using default key bindings.\n");
	}
	
	initNetwork();
	
	printf("Port: %d\n", settings.port);
	
	printf("Running on: %s\n", hostName);
	
	printf("Your local IP(s):\n");
	printIPs();
	
	printf("\n");
	
	startListening();
	
	while(1) {
		memset(&buffer, 0, sizeof(struct packet));
		
		while(receiveBuffer(sizeof(struct packet)) <= 0) {
			// Waiting
			
			Sleep(settings.throttle);
		}
		
		keyboardActive = buffer.keyboardActive;
		
		switch(buffer.command) {
			case CONNECT:
				lastKeys = 0;
				currentKeys = 0;
				circlePad.x = 0;
				circlePad.y = 0;
				lastTouch.x = 0;
				lastTouch.y = 0;
				currentTouch.x = 0;
				currentTouch.y = 0;
				
				buffer.command = CONNECT;
				printf("3DS Connected!\n");
				
				Sleep(50);
				sendBuffer(1);
				
				Sleep(50);
				sendBuffer(1);
				
				Sleep(50);
				sendBuffer(1);
				break;
			
			case KEYS:
				lastKeys = currentKeys;
				if(currentKeys & KEY_TOUCH) lastTouch = currentTouch;
				
				memcpy(&currentKeys, &buffer.keys, 4);
				memcpy(&circlePad, &buffer.circlePad, 4);
				memcpy(&currentTouch, &buffer.touch, 4);
				
				handleKey(KEY_A, settings.A);
				handleKey(KEY_B, settings.B);
				handleKey(KEY_SELECT, settings.Select);
				handleKey(KEY_START, settings.Start);
				handleKey(KEY_DRIGHT, settings.Right);
				handleKey(KEY_DLEFT, settings.Left);
				handleKey(KEY_DUP, settings.Up);
				handleKey(KEY_DDOWN, settings.Down);
				handleKey(KEY_R, settings.R);
				handleKey(KEY_L, settings.L);
				handleKey(KEY_X, settings.X);
				handleKey(KEY_Y, settings.Y);
				
				//handleKey(KEY_TOUCH, settings.Tap);
				if(release(KEY_TOUCH)) {
					simulateKeyNewpress(settings.Tap);
					simulateKeyRelease(settings.Tap);
				}
				
				//handleKey(KEY_LID, 'I');
				
				if(newpress(KEY_TOUCH)) {
					lastTouch.x = currentTouch.x;
					lastTouch.y = currentTouch.y;
				}
				
				if((currentKeys & KEY_TOUCH)) {
					if(keyboardActive) {
						if(newpress(KEY_TOUCH)) {
							char letter = currentKeyboardKey();
							if(letter) {
								simulateKeyNewpress(letter);
								simulateKeyRelease(letter);
							}
						}
					}
					else if(settings.touch == mouse) {
						if(settings.mouseSpeed) {
							POINT p;
							GetCursorPos(&p);
							SetCursorPos(p.x + (currentTouch.x - lastTouch.x) * settings.mouseSpeed, p.y + (currentTouch.y - lastTouch.y) * settings.mouseSpeed);
						}
						else {
							SetCursorPos((int)((double)currentTouch.x * widthMultiplier), (int)((double)currentTouch.y * heightMultiplier));
						}
					}
					else if(settings.touch == joystick) {
						if(vJoy) updateJoystick((currentTouch.x) * 128, (currentTouch.y) * 128);
					}
				}
				
				if(settings.circlePad == mouse) {
					if(abs(circlePad.x) < settings.mouseSpeed * 3) circlePad.x = 0;
					if(abs(circlePad.y) < settings.mouseSpeed * 3) circlePad.y = 0;
					
					POINT p;
					GetCursorPos(&p);
					SetCursorPos(p.x + (circlePad.x * settings.mouseSpeed) / 32, p.y - (circlePad.y * settings.mouseSpeed) / 32);
				}
				else if(settings.circlePad == joystick) {
					if(vJoy) updateJoystick((circlePad.x + 128) * 128, (128 - circlePad.y) * 128);
				}
				
				break;
		}
		
		sendScreenshot();
	}
	
	error("accept()");
	return 0;
}

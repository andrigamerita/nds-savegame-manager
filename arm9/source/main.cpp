#include <nds.h>
#include <stdio.h>
#include <dswifi9.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <dirent.h>
#include <algorithm>
#include <fat.h>
#include <nds/arm9/dldi.h>
#include <sys/dir.h>
#include <nds/arm9/console.h>

#include "gba.h"
#include "dsi.h"
#include "display.h"
#include "dsCard.h"
#include "hardware.h"
#include "fileselect.h"
#include "strings.h"
#include "libini.h"
#include "globals.h"

char server_address[256] = "192.168.43.1";
int server_port = 5244;
char request_text[8192] = "GET /";
char savhexdump[262144];

//==============================
Wifi_AccessPoint* findAP(void){
	int selected = 0;  
	int i;
	int count = 0, displaytop = 0;

	static Wifi_AccessPoint ap;

	Wifi_ScanMode(); //this allows us to search for APs

	int pressed = 0;
	do {

		scanKeys();
		pressed = keysDown();

		if(pressed & KEY_START) exit(0);

		//find out how many APs there are in the area
		count = Wifi_GetNumAP();


		consoleClear();

		iprintf("%d APs detected\n\n", count);

		int displayend = displaytop + 10;
		if (displayend > count) displayend = count;

		//display the APs to the user
		for(i = displaytop; i < displayend; i++) {
			Wifi_AccessPoint ap;

			Wifi_GetAPData(i, &ap);

			// display the name of the AP
			iprintf("%s %.29s\n  Wep:%s Sig:%i\n", 
				i == selected ? "*" : " ", 
				" ", ap.ssid, 
				//ap.flags & WFLAG_APDATA_WEP ? "Yes " : "No ",
				ap.rssi * 100 / 0xD0
			);

		}

		//move the selection asterick
		if(pressed & KEY_UP) {
			selected--;
			if(selected < 0) {
				selected = 0;
			}
			if(selected<displaytop) displaytop = selected;
		}

		if(pressed & KEY_DOWN) {
			selected++;
			if(selected >= count) {
				selected = count - 1;
			}
			displaytop = selected - 9;
			if (displaytop<0) displaytop = 0;
		}

		swiWaitForVBlank();
	} while(!(pressed & KEY_A));

	//user has made a choice so grab the ap and return it
	Wifi_GetAPData(selected, &ap);

	return &ap;
}

//======================
void keyPressed(int c){
	if(c > 0) iprintf("%c",c);
}

//========================
void getHttp(char* url) {
    // Let's send a simple HTTP request to a server and print the results!

    // Find the IP address of the server, with gethostbyname
    struct hostent * myhost = gethostbyname( url );
    iprintf("Found IP Address!\n");
 
    // Create a TCP socket
    int my_socket;
    my_socket = socket( AF_INET, SOCK_STREAM, 0 );
    iprintf("Created Socket!\n");

    // Tell the socket to connect to the IP address we found, on port server_port (HTTP)
    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_port = htons(server_port);
    sain.sin_addr.s_addr= *( (unsigned long *)(myhost->h_addr_list[0]) );
    connect( my_socket,(struct sockaddr *)&sain, sizeof(sain) );
    iprintf("Connected to server!\n");

    // send our request
    send( my_socket, request_text, strlen(request_text), 0 );
    iprintf("Sent our request!\n");

    // Print incoming data
    iprintf("Printing incoming data:\n");

    int recvd_len;
    char incoming_buffer[256];

    while( ( recvd_len = recv( my_socket, incoming_buffer, 255, 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
        if(recvd_len>0) { // data was received!
            incoming_buffer[recvd_len] = 0; // null-terminate
            iprintf(incoming_buffer);
		}
	}

	iprintf("Other side closed connection!");

	shutdown(my_socket,0); // good practice to shutdown the socket.

	closesocket(my_socket); // remove the socket.
}

//===============
int main(void) {
	Wifi_InitDefault(false);
	
	consoleDemoInit(); 

	Keyboard* kb = keyboardDemoInit();
	kb->OnKeyPressed = keyPressed;

	while(1) {
		int status = ASSOCSTATUS_DISCONNECTED;

		consoleClear();
		consoleSetWindow(NULL, 0,0,32,24);

		Wifi_AccessPoint* ap = findAP();

		consoleClear();
		consoleSetWindow(NULL, 0,0,32,10);

		iprintf("Connecting to %s\n", ap->ssid);

		//this tells the wifi lib to use dhcp for everything
		Wifi_SetIP(0,0,0,0,0);	
		char wepkey[64];
		int wepmode = WEPMODE_NONE;
		if (ap->flags & WFLAG_APDATA_WEP) {
			iprintf("Enter Wep Key\n");
			while (wepmode == WEPMODE_NONE) {
				scanf("%s",wepkey);
				if (strlen(wepkey)==13) {
					wepmode = WEPMODE_128BIT;
				} else if (strlen(wepkey) == 5) {
					wepmode = WEPMODE_40BIT;
				} else {
					iprintf("Invalid key!\n");
				}
			}
			Wifi_ConnectAP(ap, wepmode, 0, (u8*)wepkey);
		} else {
			Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);
		}
		consoleClear();
		while(status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT) {

			status = Wifi_AssocStatus();
			int len = strlen(ASSOCSTATUS_STRINGS[status]);
			iprintf("\x1b[0;0H\x1b[K");
			iprintf("\x1b[0;%dH%s", (32-len)/2,ASSOCSTATUS_STRINGS[status]);

			scanKeys();

			if(keysDown() & KEY_B) break;
			
			swiWaitForVBlank();
		}

		if(status == ASSOCSTATUS_ASSOCIATED) {
			u32 ip = Wifi_GetIP();

			//== After WiFi is connected ==
			//getHttp("www.akkit.org");

			while(1) {
				consoleClear();
				iprintf("\nip: [%li.%li.%li.%li]\n", (ip ) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

				iprintf("\n- START: Quit\n- SELECT: Change server address\n  (Current: ");
				iprintf(server_address);
				iprintf(":");
				//iprintf(server_port);

				iprintf(")\n- A: Dump GBA save to HTTP\n- B: GBA Backup mode\n");

				swiWaitForVBlank();
				scanKeys();
				int pressed = keysDown();

				if(pressed&KEY_START) break;

				if(pressed&KEY_B) {
					iprintf("GBA Backup mode\n");

					u8 gbatype = gbaGetSaveType();
					gbatype = gbaGetSaveType();
					hwBackupGBA(gbatype);
				}

				if(pressed&KEY_SELECT) {
					iprintf("\nWrite new server Address\n>> ");
					scanf("%s", server_address);

					iprintf("\nWrite new server Port\n>> ");
					scanf("%i", server_port);
				}

				if(pressed&KEY_A) {
					iprintf("\nConnecting to HTTP...\n");

					strcat(request_text, "?DumpSavePart=1&Content=DEAD");
					strcat(request_text, " HTTP/1.1\r\nUser-Agent: Nintendo DS\r\n\r\n");

					getHttp(server_address);

					/*iprintf("\nFTP Address:Port (Leave empty\nfor default 192.168.1.43:12345)\n>>");
					char ftpserver_address_input[256];
					scanf("%s", ftpserver_address_input);

					if (ftpserver_address_input != "") ftpserver_address[256] = ftpserver_address_input[256];
					hwLoginFTP();*/
				}
			}
		} else {
			iprintf("\nConnection failed!\n");
		}

		int quit = 0;
		iprintf("Press A to try again, B to quit.");
		while(1) {
			swiWaitForVBlank();
			scanKeys();
			int pressed = keysDown();
			if(pressed&KEY_B) quit = 1;
			if(pressed&(KEY_A|KEY_B)) break;
		}
		if(quit) break;
	}
	return 0;
}
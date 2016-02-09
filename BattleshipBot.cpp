#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <Windows.h>
#include <math.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "wsock32.lib")
#define STUDENT_NUMBER		"Day Bow Bow (chicka-chicka)"
#define STUDENT_FIRSTNAME	"Patrick"
#define STUDENT_FAMILYNAME	"McGrath"

#define UWE					"164.11.80.69"
#define HOME				"127.0.0.1"	
#define PAL					"164.11.80.90"
#define IP_ADDRESS_SERVER	UWE

#define PORT_SEND			1924	// We define a port that we are going to use.
#define PORT_RECEIVE		1925	// We define a port that we are going to use.

#define MAX_BUFFER_SIZE		500
#define MAX_SHIPS			200

#define FIRING_RANGE		100

#define MOVE_LEFT			-1
#define MOVE_RIGHT			 1
#define MOVE_UP				 1
#define MOVE_DOWN			-1
#define MOVE_FAST			 2
#define MOVE_SLOW			 1

SOCKADDR_IN	sendto_addr, receive_addr;
SOCKET		sock_send, sock_recv;  // These are our socket, it is the handle to the IO address to read/write packets
WSADATA		data;
char		InputBuffer[MAX_BUFFER_SIZE];

int myX, myY, myHealth, myFlag, number_of_ships, shipX[MAX_SHIPS], shipY[MAX_SHIPS],
shipHealth[MAX_SHIPS], shipFlag[MAX_SHIPS], shipDistance[MAX_SHIPS];

bool fire = false, moveShip = false, setFlag = true, respawn = false;
int fireX, fireY, moveX, moveY, respawnX, respawnY;

int new_flag = 310395;

int up_down = MOVE_LEFT*MOVE_FAST, left_right = MOVE_UP*MOVE_FAST;
int number_of_friends, friendX[MAX_SHIPS], friendY[MAX_SHIPS], friendHealth[MAX_SHIPS], friendFlag[MAX_SHIPS], friendDistance[MAX_SHIPS];
int number_of_enemies, enemyX[MAX_SHIPS], enemyY[MAX_SHIPS], enemyHealth[MAX_SHIPS], enemyFlag[MAX_SHIPS], enemyDistance[MAX_SHIPS];

std::vector<int> Pack = {
	310395,					//	Dylan
	260185,			 		//	Barry
	300685,					//	Alex
	270594,					//	Patrick
	220394,					//	Bill

	//456982,					//	Simon
};

//	Forward declare control functions for later definition
void fire_at_ship(int X, int Y);
void move_in_direction(int left_right, int up_down);
void set_new_flag(int newFlag);


/*
--	IsFriend	--
Args:
index - the ship in the array that is being flag-checked

Description:
If the flag flying on the ship is recognised, list as friend
*/
bool IsFriend(int index) {
	for (auto i : Pack)
		if (shipFlag[index] == i && shipFlag[index] != myFlag)
			return true;
	return false;
}


/*
--	moveTowards	--
Args:
x		- the x position to move towards
y		- the y position to move towards
speed	- the speed at which to move

Description:
Move ship towards the specified point
*/
void moveTowards(double x, double y, int speed) {
	if (myX > x)
		left_right = MOVE_LEFT*speed;
	else if (myX < x)
		left_right = MOVE_RIGHT*speed;

	if (myY > y)
		up_down = MOVE_DOWN*speed;
	else if (myY < y)
		up_down = MOVE_UP*speed;
}

/*
--	moveAwayFrom	--
Args:
x		- the x position to move away from
y		- the y position to move away from
speed	- the speed at which to move

Description:
Move ship away from the specified point
*/
void moveAwayFrom(double x, double y, int speed) {
	moveTowards(x, y, -speed);
}

/*
--	tactics	--
Description:
The procedure the battleship follows, how it reacts to different stimuli
*/
void tactics() {
	int i, convergenceX = 450, convergenceY = 450;
	const int executeThreshold = 3;

	if (number_of_ships > 1) {
		int nearestEnemyDistance = 9999, nearestEnemy = 0,
			nearestFriendDistance = 9999, nearestFriend = 0,
			packLeader = -1;

		number_of_friends = number_of_enemies = 0;

		for (i = 1; i < number_of_ships; i++) {
			shipDistance[i] = (int)ceil(sqrt(pow((double)myX - shipX[i], 2) + pow((double)myY - shipY[i], 2)));

			if (IsFriend(i)) {
				friendX[number_of_friends] = shipX[i];
				friendY[number_of_friends] = shipY[i];
				friendHealth[number_of_friends] = shipHealth[i];
				friendFlag[number_of_friends] = shipFlag[i];
				friendDistance[number_of_friends] = shipDistance[i];
				if (friendDistance[number_of_enemies] < nearestFriendDistance) {
					nearestFriendDistance = friendDistance[number_of_friends];
					nearestFriend = number_of_friends;
				}
				number_of_friends++;
			}
			else {
				enemyX[number_of_enemies] = shipX[i];
				enemyY[number_of_enemies] = shipY[i];
				enemyHealth[number_of_enemies] = shipHealth[i];
				enemyFlag[number_of_enemies] = shipFlag[i];
				enemyDistance[number_of_enemies] = shipDistance[i];
				if (enemyDistance[number_of_enemies] < nearestEnemyDistance) {
					nearestEnemyDistance = enemyDistance[number_of_enemies];
					nearestEnemy = number_of_enemies;
				}
				number_of_enemies++;
			}
		}


		if (number_of_friends > 0) {
			int speed = MOVE_FAST, avgX = 0, avgY = 0;
			bool leaderFound = false;
			for (int i = 0; i < number_of_friends; i++) {
				if (!leaderFound)
					for (auto wolf : Pack)
						if (friendFlag[i] == wolf) {
					packLeader = i;
					leaderFound = true;
						}
				avgX += friendX[i];
				avgY += friendY[i];
				if (friendHealth[i] < myHealth)
					speed = MOVE_SLOW;
			}

			avgX /= number_of_friends;
			avgY /= number_of_friends;

			int avgDistance = (int)ceil(sqrt(pow((double)myX - avgX, 2) + pow((double)myY - avgY, 2)));

			if (packLeader != -1 && avgDistance < 15) {
				if (number_of_enemies <= number_of_friends + 1)
					moveTowards(enemyX[nearestEnemy], enemyY[nearestEnemy], speed);
			}
			else moveTowards(avgX, avgY, speed);


		} // Number of friends = 0
		// If there is only one enemy and they are equally storng, or weaker than me
		else if (number_of_enemies == 1 && enemyHealth[nearestEnemy] <= myHealth)
			moveTowards(enemyX[nearestEnemy], enemyY[nearestEnemy], MOVE_FAST);
		else if (number_of_enemies > 1)
			moveAwayFrom(enemyX[nearestEnemy], enemyY[nearestEnemy], MOVE_FAST);
		else //	Default Move Code
			moveTowards(convergenceX, convergenceY, MOVE_FAST);

		if (number_of_enemies > 0 && enemyDistance[nearestEnemy] < FIRING_RANGE)
			fire_at_ship(enemyX[nearestEnemy], enemyY[nearestEnemy]);
		else if (number_of_friends > 0) {
			for (int i = 0; i < number_of_friends; i++) {
				if (friendHealth[i] <= executeThreshold)
					fire_at_ship(friendX[i], friendY[i]);
			}
		}
	}
	else {	// Still move even if there are no ships
		moveTowards(convergenceX, convergenceY, MOVE_FAST);
		if (myHealth <= executeThreshold) {
			fire_at_ship(shipX[0], shipY[0]);
		}
	}

	if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
		respawn = true;
	}

	move_in_direction(left_right, up_down);

}


/*************************************************************/
/********* Your tactics code ends here ***********************/
/*************************************************************/


void communicate_with_server() {
	char buffer[4096], chr, *p;
	bool finished;
	int  len = sizeof(SOCKADDR), i, j, rc;


	sprintf_s(buffer, "Register  %s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME);
	sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

	while (true) {
		if (recvfrom(sock_recv, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&receive_addr, &len) != SOCKET_ERROR) {
			p = ::inet_ntoa(receive_addr.sin_addr);

			if ((strcmp(IP_ADDRESS_SERVER, "127.0.0.1") == 0) || (strcmp(IP_ADDRESS_SERVER, p) == 0)) {
				i = 0;
				j = 0;
				finished = false;
				number_of_ships = 0;

				while ((!finished) && (i < 4096)) {
					chr = buffer[i];

					switch (chr) {
					case '|':
						InputBuffer[j] = '\0';
						j = 0;
						sscanf_s(InputBuffer, "%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
						number_of_ships++;
						break;

					case '\0':
						InputBuffer[j] = '\0';
						sscanf_s(InputBuffer, "%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
						number_of_ships++;
						finished = true;
						break;

					default:
						InputBuffer[j] = chr;
						j++;
						break;
					}
					i++;
				}

				myX = shipX[0];
				myY = shipY[0];
				myHealth = shipHealth[0];
				myFlag = shipFlag[0];

				tactics();

				if (fire) {
					sprintf_s(buffer, "Fire %s,%d,%d", STUDENT_NUMBER, fireX, fireY);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					fire = false;
				}

				if (moveShip) {
					sprintf_s(buffer, "Move %s,%d,%d", STUDENT_NUMBER, moveX, moveY);
					rc = sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					moveShip = false;
				}

				if (setFlag) {
					sprintf_s(buffer, "Flag %s,%d", STUDENT_NUMBER, new_flag);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					setFlag = false;
				}

				if (respawn) {
					sprintf_s(buffer, "Register  %s,%s,%s,%d,%d", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, respawnX, respawnY);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					respawn = false;
				}


			}
		}
		else
			printf_s("recvfrom error = %d\n", WSAGetLastError());

	}

	printf_s("Student %s\n", STUDENT_NUMBER);
}


void fire_at_ship(int X, int Y) {
	fire = true;
	fireX = X;
	fireY = Y;
}



void move_in_direction(int X, int Y) {
	if (X < -2) X = -2;
	if (X > 2) X = 2;
	if (Y < -2) Y = -2;
	if (Y > 2) Y = 2;

	moveShip = true;
	moveX = X;
	moveY = Y;
}


void set_new_flag(int newFlag) {
	setFlag = true;
	new_flag = newFlag;
}

int _tmain(int argc, _TCHAR* argv[]) {
	char chr = '\0';

	printf("\nBattleship Bots\nUWE Computer and Network Systems Assignment 2 (2013-14)\n");

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		return(0);

	sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_send)
		printf("Socket creation failed!\n");

	sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_recv)
		printf("Socket creation failed!\n");

	memset(&sendto_addr, 0, sizeof(SOCKADDR_IN));
	sendto_addr.sin_family = AF_INET;
	sendto_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	sendto_addr.sin_port = htons(PORT_SEND);

	memset(&receive_addr, 0, sizeof(SOCKADDR_IN));
	receive_addr.sin_family = AF_INET;
	//	receive_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	receive_addr.sin_addr.s_addr = INADDR_ANY;
	receive_addr.sin_port = htons(PORT_RECEIVE);

	int ret = bind(sock_recv, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	if (ret)
		printf("Bind failed! %d\n", WSAGetLastError());


	communicate_with_server();

	closesocket(sock_send);
	closesocket(sock_recv);
	WSACleanup();

	while (chr != '\n') {
		chr = getchar();
	}

	return 0;
}

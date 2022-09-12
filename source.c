#include <windows.h>
#include <stdio.h>

#include "vec3.h"

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"winmm.lib")

#define MAXPLAYERS 8

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

typedef struct{
	u8 lvlSz;
	u32 lmapSz;
}PROPERTIES;

typedef struct{
	u8 id;
	u8 r;
	u8 g;
	u8 b;
}MAP;

typedef struct{
	u16 r;
	u16 g;
	u16 b;
}EXRGB;

typedef struct{
	u32 p1;
	u32 p2;
	u32 p3;
	u32 p4;
	u32 p5;
	u32 p6;
	u32 p7;
	u32 p8;
	u32 p9;
	u32 p10;
	u32 p11;
	u32 p12;
}LPMAP;

typedef struct{
	VEC3 pos;
}PLAYER;

VEC3 playerPosBuf[MAXPLAYERS];
VEC3 playerVel[MAXPLAYERS];

WSADATA data;

MAP   *map;
MAP   *metadt;
MAP   *metadt2;
MAP   *metadt3;
MAP   *metadt4;
MAP   *metadt5;
MAP   *metadt6;
LPMAP *lpmap;
EXRGB *lmap;

u8 clientC;
SOCKET client[8];
SOCKET tcpSock;	
SOCKADDR_IN tcpAddress;

PROPERTIES *properties;
u32 lmapC;

PLAYER player[MAXPLAYERS];

PLAYER playerCL[MAXPLAYERS][MAXPLAYERS];

u8 playersettings[MAXPLAYERS];

u8 playerQueueC[MAXPLAYERS];
u8 playerQueue[256][MAXPLAYERS];

i32 sockret[MAXPLAYERS];

void serverRecvTimer(u8 id){
	Sleep(15);
	while(sockret[id]!=0){
		//VEC3addVEC3(&player[id],playerVel[id]);
		Sleep(15);
	}
}

void serverSend(u8 *clientID){
	u8 id = *clientID;
	u8 packetID = 0;
	for(;;){
		u32 i2 = 0;
		for(u32 i = 0;i < MAXPLAYERS;i++){
			if(client[i] && i != id){
				playerCL[id][i2] = player[i];
				i2++;
			}
		}
		while(playerQueueC[id]){
			playerQueueC[id]--;
			packetID = playerQueue[playerQueueC[id]][id];
			switch(packetID){
			case 1:
			case 2:
				send(client[id],&packetID,1,0);
				break;
			}
		}
		packetID = 0;
		send(client[id],&packetID,1,0);
		send(client[id],playerCL[id],sizeof(PLAYER)*(clientC-1),0);
		Sleep(15);
	}
}

void serverRecv(u8 id){
	for(;;){
		serverRecvTimer(id);
		sockret[id] = recv(client[id],&player[id],sizeof(PLAYER),0);
		if(sockret[id] == -1 || sockret[id] == 0){
			if(WSAGetLastError() == WSAECONNRESET){
				closesocket(client[id]);
				client[id] = 0;
				clientC--;
				for(u32 i = 0;i < MAXPLAYERS;i++){
					if(client[i]){
						playerQueue[playerQueueC[i]][i] = 2;
						playerQueueC[i]++;
					}
				}
				printf("clientDisconnect\n");
				return;
			}
		}
		playerVel[id] = VEC3subVEC3R(player[id].pos,playerPosBuf[id]);
		playerPosBuf[id] = player[id].pos;
		sockret[id] = 0;
	}
}

void mapSend(u8 *clientID){
	u8 id = *clientID;
	send(client[id],&properties->lvlSz,1,0);
	send(client[id],&properties->lmapSz,4,0);
	send(client[id],&clientC,1,0,0);
	send(client[id],&id,1,0);
	send(client[id],&lmapC,4,0,0);
	send(client[id],map,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt2,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt3,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt4,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt5,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],metadt6,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0);
	send(client[id],lpmap,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(LPMAP),0);
	send(client[id],lmap,lmapC*properties->lmapSz*properties->lmapSz*sizeof(EXRGB),0,0);
	printf("clientLoaded\n");
	CreateThread(0,0,serverSend,&id,0,0);
	serverRecv(id);
}

u8 searchServerSlot(){
	u8 r = 0;
	while(client[r]){
		r++;
	}
	return r;
}

void main(){
	timeBeginPeriod(1);
	properties = HeapAlloc(GetProcessHeap(),8,sizeof(PROPERTIES));
	HANDLE h = CreateFileA("level.lvl",GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	ReadFile(h,&properties->lvlSz,1,0,0);
	ReadFile(h,&properties->lmapSz,4,0,0);
	map        = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt     = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt2    = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt3    = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt4    = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt5    = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	metadt6    = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(MAP));
	lpmap      = HeapAlloc(GetProcessHeap(),8,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(LPMAP));
	ReadFile(h,map,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt2,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt3,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt4,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt5,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,metadt6,properties->lvlSz*properties->lvlSz*properties->lvlSz*4,0,0);
	ReadFile(h,lpmap,properties->lvlSz*properties->lvlSz*properties->lvlSz*sizeof(LPMAP),0,0);
	ReadFile(h,&properties->lmapSz,1,0,0);
	ReadFile(h,&lmapC,4,0,0);
	lmap = HeapAlloc(GetProcessHeap(),8,lmapC*properties->lmapSz*properties->lmapSz*sizeof(EXRGB));
	ReadFile(h,lmap,lmapC*properties->lmapSz*properties->lmapSz*sizeof(EXRGB),0,0);
	CloseHandle(h);
	WSAStartup(MAKEWORD(2, 2),&data);
	tcpSock = socket(AF_INET,SOCK_STREAM,0);

	tcpAddress.sin_family = AF_INET;
	tcpAddress.sin_port = htons(7778);
	tcpAddress.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(tcpSock,(SOCKADDR*)&tcpAddress,sizeof(tcpAddress));

	for(;;){
		listen(tcpSock,SOMAXCONN);
		SOCKET temp = accept(tcpSock,0,0);
		u8 id = searchServerSlot();
		for(u32 i = 0;i < MAXPLAYERS;i++){
			if(client[i]){	
				playerQueue[playerQueueC[i]][i] = 1;
				playerQueueC[i]++;
			}
		}
		client[id] = temp;
		clientC++;
		CreateThread(0,0,mapSend,&id,0,0);
	}
}
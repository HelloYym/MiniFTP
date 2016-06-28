//
//  main.cpp
//  FtpServer
//
//  Created by 杨煜溟 on 16/5/27.
//  Copyright © 2016年 Yym. All rights reserved.
//

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <pthread.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using std::cout;
using std::endl;
using std::string;
using std::cin;
using std::ifstream;
using std::istringstream;
using std::ios;
using std::ofstream;

#define SERVER_PORT	6666 //侦听端口
#define DT_PORT 20000 //数据端口
#define BUFFER_SIZE 1000
#define ROOT_PATH "."
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define IP1 127
#define IP2 0
#define IP3 0
#define IP4 1

enum {
	NEEDPASS = 331,
	LOGGEDIN = 230,
	OPTNOSUPPORT = 501,
	PATHEXSIT = 257,
	CMDSUCCESS = 250,
	PASSMODE = 227,
	TRANBEGN = 125,
	TRANCOMP = 226,
	ACTIONOK = 200,
	TIMEOUT = 441,
	NOTLOG = 230,
	PATHNOTEXIST = 550,
	FILESTATUS = 213
};
const int timeout = 120000;
int dtListen;
sockaddr_in dtaServer;

void* CreateConnect(void *lpParameter) {
	int* sServer = (int*) lpParameter;
	*sServer = accept(dtListen, NULL, NULL);
	return 0;
}

void* ClientThread(void *lpParameter) {
	string current_path = "/";
	struct sockaddr addr;
	struct sockaddr_in *addr_v4;
	int sServer = *(int*) lpParameter, dtServer;
	char buffer[BUFFER_SIZE + 1], ret_str[BUFFER_SIZE + 1];
	string act, arg;
	int addr_len = sizeof(addr), port, filesize;
	int ret;
	while (1) {
		if (recv(sServer, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
			cout << "recv failed" << endl;
			break;
		}
		istringstream in(buffer);
		in >> act >> arg;
		if (act == "PWD") {
			sprintf(ret_str, "%d \"%s\" is current directory.\r\n", PATHEXSIT,
					current_path.c_str());
			if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
				cout << "PWD failed" << endl;
				continue;
			}
		} else if (act == "CWD") {
			if (access((ROOT_PATH + arg).c_str(), 0) != 0) {
				sprintf(ret_str,
						"%d %s: The system cannot find the file specified.\r\n",
						PATHNOTEXIST, (ROOT_PATH + arg).c_str());
				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "CD failed" << endl;
					continue;
				}
			} else {
				current_path = arg;
				sprintf(ret_str, "%d CWD command successful.\r\n", CMDSUCCESS);
				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "CD failed" << endl;
					continue;
				}
			}
		} else if (act == "PASV") {
			//			int old = dtServer = dtListen;

			pthread_t hThread;
			pthread_create(&hThread, NULL, CreateConnect, &dtServer);

			if (ret == SOCKET_ERROR) {
				cout << "bind failed" << endl;
				continue;
			}

			if (getsockname(dtListen, (struct sockaddr*) &addr,
					(socklen_t *) &addr_len) == 0) {
				if (addr.sa_family == AF_INET) {
					addr_v4 = (sockaddr_in*) &addr;
					port = ntohs(addr_v4->sin_port);
					sprintf(ret_str,
							"%d Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n",
							PASSMODE, IP1, IP2, IP3, IP4, port / 256,
							port % 256);

					if (send(sServer, ret_str, (size_t) strlen(ret_str),
							0) == SOCKET_ERROR) {
						cout << "PASV failed" << endl;
						continue;
					}
				}

			} else {
				continue;
			}
		} else if (act == "SIZE") {
			if (access((ROOT_PATH + current_path + arg).c_str(), 0) != 0) {
				sprintf(ret_str,
						"%d %s: The system cannot find the file specified.\r\n",
						PATHNOTEXIST, (current_path + arg).c_str());

				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "SIZE failed" << endl;
					continue;
				}
			} else {
				ifstream in;
				in.open(ROOT_PATH + current_path + "/" + arg, std::ios::binary);
				in.seekg(0, in.end);
				filesize = in.tellg();
				in.seekg(0, in.beg);
				sprintf(ret_str, "%d %d\r\n", FILESTATUS, filesize);
				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "SIZE failed" << endl;
					continue;
				}
				in.close();
			}
		} else if (act == "RETR") {
			if (access((ROOT_PATH + current_path + "/" + arg).c_str(), 0)
					!= 0) {
				sprintf(ret_str,
						"%d %s: The system cannot find the file specified.\r\n",
						PATHNOTEXIST, (current_path + arg).c_str());

				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "RETN failed" << endl;
					continue;
				}
			} else {
				sprintf(ret_str,
						"%d Data connection already open; Transfer starting.\r\n",
						TRANBEGN);
				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "RETN failed" << endl;
					continue;
				}
				ifstream in(ROOT_PATH + current_path + "/" + arg, ios::binary);
				in.seekg(0, in.end);
				filesize = in.tellg();
				in.seekg(0, in.beg);
				while (filesize > 0) {
					in.read(buffer, BUFFER_SIZE);
					send(dtServer, buffer,
							filesize > BUFFER_SIZE ? BUFFER_SIZE : filesize, 0);
					filesize -= BUFFER_SIZE;
				}
				in.close();
				sprintf(ret_str, "%d Transfer complete.\r\n", TRANCOMP);
				if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
					cout << "RETN failed" << endl;
					continue;
				}
				close(dtServer);
			}
		} else if (act == "STOR") {
			sprintf(ret_str,
					"125 Data connection already open; Transfer starting.\r\n");
			if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
				cout << "RETN failed" << endl;
				continue;
			}
//			ofstream out(ROOT_PATH + current_path + "/" + arg, ios::binary);
			ofstream out(ROOT_PATH + current_path + arg, ios::binary);
			cout << ROOT_PATH + current_path + "/" + arg << endl;
			while (1) {
				int ret = recv(dtServer, buffer, BUFFER_SIZE, 0);
				if (ret == SOCKET_ERROR || ret == 0) {
					break;
				}
				out.write(buffer, ret);
			}
			out.close();
			sprintf(ret_str, "%d Transfer complete.\r\n", TRANCOMP);
			if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
				cout << "RETN failed" << endl;
				continue;
			}
			close(dtServer);
		} else {
			sprintf(ret_str, "001 Unknown command.\r\n");
			if (send(sServer, ret_str, strlen(ret_str), 0) == SOCKET_ERROR) {
				cout << "RETN failed" << endl;
				continue;
			}
		}
	}
	cout << "socket id " << sServer << " quit" << endl;
	return 0;
}

int main(int argc, const char * argv[]) {

	int ret, length;
	int sListen, sServer; //侦听套接字，连接套接字
	struct sockaddr_in saServer, saClient; //地址信息

	//创建socket，使用TCP协议：
	sListen = socket(AF_INET, SOCK_STREAM, 0);
	dtListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET || dtListen == INVALID_SOCKET) {
		cout << "socket() failed!" << endl;
		return 0;
	}

	//构建本地地址信息：
	memset(&saServer, 0, sizeof(saServer));
	saServer.sin_family = AF_INET; //地址家族
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络字节序
	saServer.sin_addr.s_addr = htonl(INADDR_ANY); //使用INADDR_ANY指示任意地址

	dtaServer.sin_family = AF_INET; //地址家族
	dtaServer.sin_port = htons(DT_PORT); //注意转化为网络字节序
	dtaServer.sin_addr.s_addr = htonl(INADDR_ANY); //使用INADDR_ANY指示任意地址s

	//绑定：
	bind(sListen, (struct sockaddr *) &saServer, sizeof(saServer));

	bind(dtListen, (struct sockaddr *) &dtaServer, sizeof(dtaServer));

	//侦听连接请求：
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR) {
		cout << "listen() failed!" << endl;
		close(sListen); //关闭套接字
		return 0;
	}
	//侦听连接请求：
	ret = listen(dtListen, 5);
	if (ret == SOCKET_ERROR) {
		cout << "listen() failed!" << endl;
		close(dtListen); //关闭套接字
		return 0;
	}

	cout << "Waiting for client connecting!" << endl;
	cout << "tips : Ctrl+c to quit!" << endl;

	while (true) {
		//阻塞等待接受客户端连接：
		length = sizeof(saClient);
		sServer = accept(sListen, (struct sockaddr *) &saClient,
				(socklen_t *) &length);

		cout << "new client" << endl;

		if (sServer == INVALID_SOCKET) {
			cout << "accept() failed!" << endl;
			continue;
		}

		cout << "Accepted client: " << inet_ntoa(saClient.sin_addr) << ":"
				<< ntohs(saClient.sin_port) << endl;

		pthread_t hThread;
		pthread_create(&hThread, NULL, ClientThread, &sServer);

		if (hThread == NULL) {
			printf("Create Thread Failed!\n");
			break;
		}

		char title[] = "220 welcome to yym's ftp server.\r\n";
		send(sServer, title, strlen(title), 0);
	}

	close(sListen); //关闭套接字
	close(sServer);

}


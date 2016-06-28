//
//  main.cpp
//  FtpClient
//
//  Created by 杨煜溟 on 16/5/26.
//  Copyright © 2016年 Yym. All rights reserved.
//

#include <iostream>
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

using namespace std;

#define SERVER_PORT 6666 //侦听端口
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_LEN 1000
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

enum MESG_VALUE {
	TIMEOUT = 421
};

bool need_mesg = false, socket_closed;
int semaphore = 1;

int code;
string ans;

void ftp_wait(volatile bool &flag) {
	flag = true;
	while (flag) {
		usleep(100);
	}
}
void ftp_wait(volatile int &semaphore) {
	semaphore -= 1;
	while (semaphore < 0) {
		usleep(100);
	}
}

void ftp_signal(bool &flag) {
	flag = false;
}

void ftp_signal(int &semaphore) {
	semaphore++;
}

vector<string> getInput() {
	static char input[BUFFER_LEN];
	vector<string> ret;
	string tmp;
	cin.getline(input, BUFFER_LEN);
	istringstream in(input);
	while (!in.eof()) {
		in >> tmp;
		if (tmp == "")
			continue;
		ret.push_back(tmp);
	}
	return ret;
}

void* recvMessage(void *lpParameter) {

	int sClient = *(int*) lpParameter;
	int ret;
	char recv_mesg[BUFFER_LEN + 1], ans_mesg[BUFFER_LEN + 1];
	while (1) {
		if (socket_closed)
			ftp_wait(socket_closed);

		ret = recv(sClient, recv_mesg, (size_t) BUFFER_LEN, 0);

		if (ret == -1) {
			cout
					<< "recv() failed!Server may be shutdown or the network is unstable.\n"
					<< endl;
			exit(0);
		}
		recv_mesg[ret] = 0;
		cout << recv_mesg;
		istringstream ss(recv_mesg);
		ss >> code;
		ss.getline(ans_mesg, BUFFER_LEN, '\r');
		if (code == TIMEOUT) {
			ftp_wait(semaphore);
			close(sClient);
			socket_closed = true;
			ftp_signal(semaphore);
		} else if (need_mesg) {
			ans = ans_mesg;
			ftp_signal(need_mesg);
		}
	}
}

int main(int argc, const char * argv[]) {

	char recv_mesg[BUFFER_LEN + 1];

	int ret;
	int sClient, dtClient; //连接套接字
	struct sockaddr_in saServer, dtServer; //地址信息
	pthread_t hThread = NULL;

	string order, username, password, directory, buff, desfile, savefile, addr;
	int ip0, ip1, ip2, ip3, port0, port1, filesize;

	//创建socket，使用TCP协议：
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == -1) {
		cout << "socket() failed!" << endl;
		return 0;
	}

	//构建服务器地址信息：
	saServer.sin_family = AF_INET; //地址家族
	dtServer.sin_family = AF_INET;
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络字节序
	saServer.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	//连接服务器：
	ret = connect(sClient, (struct sockaddr *) &saServer, sizeof(saServer));
	if (ret == -1) {
		cout << "connect() failed!" << endl;
		close(sClient); //关闭套接字
		return 0;
	}

	socket_closed = false;

	pthread_create(&hThread, NULL, recvMessage, &sClient);

	ftp_wait(need_mesg);

	while (1) {
		cout << "ftp>";
		vector<string> input = getInput();  // 等待用户输入
		if (input.size() == 0)
			continue;
		order = input[0];
		if (socket_closed) {
			ftp_wait(semaphore);
			//连接服务器：
			sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sClient == -1) {

				cout << "socket() failed!" << endl;
				return 0;
			}

			ret = connect(sClient, (struct sockaddr *) &saServer,
					sizeof(saServer));
			if (ret == -1) {
				cout << "connect() failed!" << endl;
				close(sClient);  //关闭套接字
				return 0;
			}
			ftp_signal(semaphore);
			ftp_signal(socket_closed);
		}
		if (order == "pwd") {
			ret = send(sClient, "PWD\r\n", 5, 0);
			if (ret == SOCKET_ERROR) {
				cout << "pwd failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);    // 等待服务器返回消息
		} else if (order == "user") {
			if (input.size() < 2) {
				cout << "usage:user username" << endl;
				continue;
			}
			username = input[1];
			buff = "USER " + username + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
		} else if (input[0] == "password") {
			if (input.size() < 2) {
				cout << "usage:password password" << endl;
				continue;
			}
			password = input[1];
			buff = "PASS " + password + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
		} else if (order == "cd") {
			if (input.size() < 2) {
				cout << "usage:cd directory" << endl;
				continue;
			}
			directory = input[1];
			ret = send(sClient, "PWD\r\n", 5, 0);
			if (ret == SOCKET_ERROR) {
				cout << "pwd failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
			buff = ans;
			buff = buff.substr(buff.find("\"") + 1);
			buff = buff.substr(0, buff.find("\""));
			buff = "CWD " + buff;
			if (buff != "CWD /")
				buff += "/";
			buff += directory + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
		} else if (order == "get") {
			if (input.size() != 3) {
				cout << "usage:get desfilename savefilename" << endl;
				continue;
			}
			desfile = input[1];
			savefile = input[2];
			ret = send(sClient, "TYPE I\r\n", 8, 0);
			if (ret == SOCKET_ERROR) {
				cout << "TYPE I failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);

			//改为pasv模式，获得文件传输的端口号
			ret = send(sClient, "PASV\r\n", 6, 0);
			if (ret == SOCKET_ERROR) {
				cout << "PASV failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
			addr = ans;
			addr = addr.substr(addr.find("(") + 1);
			addr = addr.substr(0, addr.find(")"));
			sscanf(addr.c_str(), "%d,%d,%d,%d,%d,%d", &ip0, &ip1, &ip2, &ip3,
					&port0, &port1);
			dtServer.sin_port = htons(port0 * 256 + port1);
			dtServer.sin_addr.s_addr = ((ip3 * 256 + ip2) * 256 + ip1) * 256
					+ ip0;

			//获得文件大小
			buff = "SIZE " + desfile + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
			if (code == 550)
				continue;
			sscanf(ans.c_str(), "%d", &filesize);

			//根据返回值建立一个新的套接字连接
			dtClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (dtClient == INVALID_SOCKET) {
				cout << "socket() failed!" << endl;
				return 0;
			}

			ret = connect(dtClient, (struct sockaddr *) &dtServer,
					sizeof(dtServer));
			if (ret == SOCKET_ERROR) {
				cout << "connect() failed!" << endl;
				close(dtClient);    //关闭套接字
				continue;
			}
			//get文件
			buff = "RETR " + desfile + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
			if (code == 550) {
				close(dtClient);
				continue;
			}

			//文件下载
			ofstream out(savefile, ios_base::binary);
			while (filesize > 0) {
				ret = recv(dtClient, recv_mesg, BUFFER_LEN, 0);
				out.write(recv_mesg, ret);
				filesize -= ret;
			}
			close(dtClient);
			out.close();
		} else if (order == "put") {
			if (input.size() != 3) {
				cout << "usage:put desfilename savefilename" << endl;
				continue;
			}
			desfile = input[1];
			savefile = input[2];
			ret = send(sClient, "TYPE I\r\n", 8, 0);
			if (ret == SOCKET_ERROR) {
				cout << "TYPE I failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);

			//改为pasv模式，获得文件传输的端口号
			ret = send(sClient, "PASV\r\n", 6, 0);
			if (ret == SOCKET_ERROR) {
				cout << "PASV failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);
			addr = ans;
			addr = addr.substr(addr.find("(") + 1);
			addr = addr.substr(0, addr.find(")"));
			sscanf(addr.c_str(), "%d,%d,%d,%d,%d,%d", &ip0, &ip1, &ip2, &ip3,
					&port0, &port1);
			dtServer.sin_port = htons(port0 * 256 + port1);
			dtServer.sin_addr.s_addr = ((ip3 * 256 + ip2) * 256 + ip1) * 256
					+ ip0;

			//根据返回值建立一个新的套接字连接
			dtClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (dtClient == INVALID_SOCKET) {
				cout << "socket() failed!" << endl;
				return 0;
			}

			ret = connect(dtClient, (struct sockaddr *) &dtServer,
					sizeof(dtServer));
			if (ret == SOCKET_ERROR) {
				cout << "connect() failed!" << endl;
				close(dtClient);    //关闭套接字
				continue;
			}

			//put文件
			buff = "STOR " + savefile + "\r\n";
			ret = send(sClient, buff.c_str(), buff.length(), 0);
			if (ret == SOCKET_ERROR) {
				cout << buff << " failed!" << endl;
				break;
			}
			ftp_wait(need_mesg);

			ifstream in(desfile, ios::binary);
			// get length of file:
			in.seekg(0, in.end);
			filesize = in.tellg();
			in.seekg(0, in.beg);
			while (filesize > 0) {
				in.read(recv_mesg, BUFFER_LEN);
				ret = send(dtClient, recv_mesg,
				BUFFER_LEN > filesize ? filesize : BUFFER_LEN, 0);
				filesize -= BUFFER_LEN;
			}
			in.close();
			close(dtClient);

		} else if (order == "quit") {
			break;
		} else {
			cout << "unknown command." << endl;
			continue;
		}
	}

	cout << "bye" << endl;
	pthread_kill(hThread, 0);
	close(sClient);    //关闭套接字
}

# MiniFTP

### 4.1 Socket 函数库

4.1.1 socket()函数
	int  socket(int protofamily, int type, int protocol);

socket()用于创建一个socket描述符（socket descriptor），它唯一标识一个socket。当我们调用socket创建一个socket时，返回的socket描述字它存在于协议族空间中，但没有一个具体的地址。如果想要给它赋值一个地址，就必须调用bind()函数，否则就当调用connect()、listen()时系统会自动随机分配一个端口。

4.1.2 bind()函数
	int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

bind()函数把一个地址族中的特定地址赋给socket。例如对应AF_INET、AF_INET6就是把一个ipv4或ipv6地址和端口号组合赋给socket。
函数的三个参数分别为：
sockfd：即socket描述字，它是通过socket()函数创建了，唯一标识一个socket。bind()函数就是将给这个描述字绑定一个名字。
addr：一个const struct sockaddr *指针，指向要绑定给sockfd的协议地址。这个地址结构根据地址创建socket时的地址协议族的不同而不同，如ipv4对应的是：
	struct sockaddr_in {
	  sa_family_t    sin_family; /* address family: AF_INET */
	  in_port_t      sin_port;   /* port in network byte order */
	  struct in_addr sin_addr;   /* internet address */
	};
	
	/* Internet address. */
	  struct in_addr {
	  uint32_t       s_addr;     /* address in network byte order 	*/
	}; 
	addrlen：对应的是地址的长度。
​	
通常服务器在启动的时候都会绑定一个地址，用于提供服务，客户就可以通过它来接连服务器；而客户端就不用指定，有系统自动分配一个端口号和自身的ip地址组合。通常服务器端在listen之前会调用bind()，而客户端就不会调用，而是在connect()时由系统随机生成一个。

4.1.3 listen()、connect()函数
	int listen(int sockfd, int backlog);
	int connect(int sockfd, const struct sockaddr *addr, 			socklen_t addrlen);

如果作为一个服务器，在调用 socket()、bind()之后就会调用listen()来监听这个socket，如果客户端这时调用 connect() 发出连接请求，服务器端就会接收到这个请求。
listen 函数的第一个参数即为要监听的 socket 描述字，第二个参数为相应 socket可以排队的最大连接个数。socket() 函数创建的 socket 默认是一个主动类型的，listen函数将 socket 变为被动类型的，等待客户的连接请求。
connect 函数的第一个参数即为客户端的 socket 描述字，第二参数为服务器的socket地址，第三个参数为 socket 地址的长度。客户端通过调用 connect 函数来建立与 TCP 服务器的连接。

4.1.4 accept()函数
	int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

TCP服务器端依次调用 socket()、bind()、listen() 之后，就会监听指定的 socket 地址了。TCP 客户端依次调用 socket()、connect() 之后就向 TCP 服务器发送了一个连接请求。TCP 服务器监听到这个请求之后，就会调用 accept() 函数取接收请求，这样连接就建立好了。之后就可以开始网络 I/O 操作了，即类同于普通文件的读写I/O操作。

参数sockfd：上面解释中的监听套接字，这个套接字用来监听一个端口，当有一个客户与服务器连接时，它使用这个一个端口号，而此时这个端口号正与这个套接字关联。当然客户不知道套接字这些细节，它只知道一个地址和一个端口号。

参数addr：这是一个结果参数，它用来接受一个返回值，这返回值指定客户端的地址，当然这个地址是通过某个地址结构来描述的，用户应该知道这一个什么样的地址结构。如果对客户的地址不感兴趣，那么可以把这个值设置为NULL。

参数len：如同大家所认为的，它也是结果的参数，用来接受上述addr的结构的大小的，它指明addr结构所占有的字节个数。同样的，它也可以被设置为NULL。

如果accept成功返回，则服务器与客户已经正确建立连接了，此时服务器通过accept返回的套接字来完成与客户的通信。

4.1.5 read()、write()等函数
	 #include <unistd.h>

	ssize_t read(int fd, void *buf, size_t count);
	ssize_t write(int fd, const void *buf, size_t count);
	
	#include <sys/types.h>
	#include <sys/socket.h>
	
	ssize_t send(int sockfd, const void *buf, size_t len, int flags);
	ssize_t recv(int sockfd, void *buf, size_t len, int flags);
	
	ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
			const struct sockaddr *dest_addr, socklen_t addrlen);
	ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
			struct sockaddr *src_addr, socklen_t *addrlen);
	
	ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
	ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

4.1.6 close()函数
	int close(int fd);
在服务器与客户端建立连接之后，会进行一些读写操作，完成了读写操作就要关闭相应的socket描述字，好比操作完打开的文件要调用fclose关闭打开的文件。


4.2 Socket 编程步骤

服务器端编程的步骤：
创建套接字：socket()
绑定套接字到一个IP地址和一个端口上：bind()
将套接字设置为监听模式等待连接请求：listen()
请求到来后，接受连接请求，返回一个新的对应于此次连接的套接字：accept()
用返回的套接字和客户端进行通信：send()/recv()
返回，等待另一连接请求；
关闭套接字：closesocket()

客户端编程的步骤：
创建套接字：socket()
向服务器发出连接请求：connect()
和服务器端进行通信：send()/recv()
关闭套接字：closesocket()

示意图：




4.3 FTP 协议
FTP（File Transfer Protocol，文件传输协议） 是 TCP/IP 协议组中的协议之一。FTP协议包括两个组成部分，其一为FTP服务器，其二为FTP客户端。其中FTP服务器用来存储文件，用户可以使用FTP客户端通过FTP协议访问位于FTP服务器上的资源。
默认情况下FTP协议使用TCP端口中的 20和21这两个端口，其中20用于传输数据，21用于传输控制信息。但是，是否使用20作为传输数据的端口与FTP使用的传输模式有关，如果采用主动模式，那么数据传输端口就是20；如果采用被动模式，则具体最终使用哪个端口要服务器端和客户端协商决定。
FTP支持两种模式，一种方式叫做Standard (也就是 PORT方式，主动方式)，一种是 Passive(也就是PASV，被动方式)。 Standard模式 FTP的客户端发送 PORT 命令到FTP服务器。Passive模式FTP的客户端发送 PASV命令到 FTP Server。

4.3.1 访问控制命令

USER：参数是标记用户的Telnet串。用户标记是访问服务器必须的，此命令通常是控制连接后第一个发出的命令。服务器可以在任何时间接收新的USER命令以改变访问控制和帐户信息。

PASS：数是标记用户口令的Telnet串。此命令紧跟USER命令，在某些站点它是完成访问控制不可缺少的一步。因此口令是个重要的东西，因此不能显示出来，服务器方没有办法隐藏口令，所以这一任务得由用户FTP进程完成。

CWD：此命令使用户可以在不同的目录或数据集下工作而不用改变它的登录或帐户信息。传输参数也不变。参数一般是目录名或与系统相关的文件集合。

QUIT：此命令终止USER，如果没有数据传输，服务器关闭控制连接；如果有数据传输，在得到传输响应后服务器关闭控制连接。

4.3.2 传输参数命令

PORT：参数是要使用的数据连接端口，通常情况下对此不需要命令响应。如果使用此命令时，要发送32位的IP地址和16位的TCP端口号。

PASV：此命令要求服务器DTP在指定的数据端口侦听，进入被动接收请求的状态，参数是主机和端口地址。

4.3.3 FTP服务命令

RETR：此命令使服务器DTP传送指定路径内的文件复本到服务器或用户DTP。这边服务器上文件的状态和内容不受影响。

STOR：
此命令使服务器DTP接收数据连接上传送过来的数据，并将数据保存在服务器的文件中。如果文件已存在，原文件将被覆盖。如果文件不存在，则新建文件。

PWD：在响应是返回当前工作目录。

4.3.4 FTP 应答
FTP命令的响应是为了对数据传输请求和过程进行同步，也是为了让用户了解服务器的状态。每个命令必须有最少一个响应，如果是多个，它们要易于区别。有些命令是有顺序性的，因此其中任何一个命令的失败会导致从头开始。FTP响应由三个数字构成，后面是一些文本。数字带有足够的信息命名用户PI不用检查文本就知道发生了什么。

4.3.5 控制连接
服务器协议解释器会在端口 L 侦听，用户或用户协议解释器初始化全双工控制连接，服务器和用户进程应该遵守 Telnet 协议的说明进行。 FTP控制连接通过用户进程端口U和服务器端口L建立，这里默认的L=21。
	服务器不提供对命令行的编辑功能，应该由用户负责这一切。在全部传送和应答结束后，在用户的请求下服务器关闭控制连接。
4.3.6 数据连接
传送数据机制包括建立连接选择数据参数。用户和服务器 DTP 有默认数据端口。用户进程默认数据端口和控制连接端口相同。服务器进程默认数据端口和控制连接端口相邻。

被动数据传输进程在数据端口接收数据，FTP请求命令决定数据传输的方向。服务器在接收到请求以后，将初始化端口的数据连接。当连接建立后，传输在DTP之间传送，服务器PI对用户PI返回应答。FTP实现运行一个默认数据端口，用户PI才能改变默认端口。

通过PORT命令可能改变端口，用户可能希望数据在第三方主机上进行其它操作，用户PI需要在两个服务器PI上建立连接。一个服务器被告知侦听另一服务器的请求。用户PI通过PORT命令通知另一服务器的数据端口。最后双方发送相应的传送命令。

每个客户端的连接请求在服务器上开启一个控制线程，当传输请求发生时，初始化socket 监听数据端口，客户端相应地开启一个 socket 连接到服务器的数据传输端口。
#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#else
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#endif

enum MsgType {
	SR_CONTENT,
	SR_QUIT,
	SR_UNKOWN = 0xff
};

#define SIZE_4K  4096
struct SocketHeader {
	int type_	= SR_UNKOWN;
	int length_ = 0;
};
const int header_size = sizeof(SocketHeader);

struct MsgBuffer {
	char* data_ = nullptr;
	int length_ = 0;
};


class SRSocketClient {
public:
	SRSocketClient(int connect_id);
	~SRSocketClient();

	void run();

	bool is_ok();

	static void socket_thread_proc(void* self_ptr);

	static void sr_thread_proc(void* self_ptr);
private:
	void wav_proc(MsgBuffer buffer);
private:
	int		con_id_ = -1;
	bool	running_ = true;	
	std::thread thread_;
	std::thread sr_thread_;
	std::mutex	mutex_;

	std::queue<MsgBuffer>   content_list;
};

typedef std::vector<SRSocketClient*> SocketClients;


class SRSocketSever
{
public:
	explicit  SRSocketSever(int cnt,int port);
	~SRSocketSever();

	void start();

private:
	int		client_count_ = 5;
	SocketClients	clients_;
	int		port_;
};




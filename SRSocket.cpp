#include "SRSocket.h"
//#include <WS2tcpip.h>
#include <iostream>
#include "SRHelper.h"
#include <fstream>
#include <string.h>
//#include<winsock2.h>


typedef std::vector<int> IntArray;

MsgBuffer make_buffer(int lenth)
{
	MsgBuffer buffer;
	buffer.data_ = new char[lenth];
	memset(buffer.data_,0, lenth);
	buffer.length_ = lenth;
	return buffer;
}


SRSocketClient::SRSocketClient(int connect_id)
	:con_id_(connect_id)
{

}

SRSocketClient::~SRSocketClient()
{
	if (thread_.joinable())
	{
		thread_.join();
	}
	if (sr_thread_.joinable())
	{
		sr_thread_.join();
	}
}

void SRSocketClient::run()
{
	running_ = true;
	thread_ = std::thread(socket_thread_proc, this);
	sr_thread_ = std::thread(sr_thread_proc, this);
}

void SRSocketClient::socket_thread_proc(void* self_ptr)
{
	SRSocketClient* self = (SRSocketClient*)self_ptr;
	while (self->running_)
	{
		char buf[header_size];
		recv(self->con_id_, buf, header_size, MSG_WAITALL);
		SocketHeader header;
		memcpy(&header, buf, header_size);
		if (header.type_ == SR_CONTENT)
		{
			std::cout << "thread " << self->con_id_ << "recv content msg length=" <<header.length_<< std::endl;
			if (header.length_)
			{
				//循环读取内容
				int pack_cnt = header.length_ / SIZE_4K;
				IntArray packages(pack_cnt, SIZE_4K);
				int last = header.length_%SIZE_4K;
				if (last > 0)
				{
					packages.push_back(last);
				}
				MsgBuffer msg_buffer = make_buffer(header.length_);
				char pack_buf[SIZE_4K];
				memset(pack_buf, 0, SIZE_4K);
				int pos = 0;
				for (int i = 0; i < packages.size();i++)
				{
					int ret =  recv(self->con_id_, pack_buf, packages[i], MSG_WAITALL);
					memcpy(msg_buffer.data_ +pos, pack_buf, packages[i]);
					pos += packages[i];
				}
				self->mutex_.lock();
				self->content_list.push(msg_buffer);
				self->mutex_.unlock();
			}
		}
		else if (header.type_ == SR_QUIT)
		{
			self->running_ = false;
			close(self->con_id_);
		}
	}
}

void SRSocketClient::sr_thread_proc(void* self_ptr)
{
	SRSocketClient* self = (SRSocketClient*)self_ptr;
	while (self->running_)
	{
		if (self->content_list.size())
		{
			std::cout << "thread " << self->con_id_ << "start proc wav" << std::endl;
			self->mutex_.lock();
			MsgBuffer buffer = self->content_list.front();
			self->content_list.pop();
			self->mutex_.unlock();
			//////////语音识别
			//
			self->wav_proc(buffer);
		}
	}
}

void SRSocketClient::wav_proc(MsgBuffer buffer)
{
	char wav_info_header[16];
	memcpy(wav_info_header, buffer.data_, 16);
	//char* result =sr_proc()
	std::string path = "data/wav/" + std::to_string(con_id_) + ".wav";
	std::ofstream fout(path);
	fout.write(buffer.data_ + 16, buffer.length_ - 16);
	fout.close();
	int a = SRHelper::instance()->a;


	std::string result;
	//.do some proc;
	char* retrun_buffer = new char[result.length() + 16];

	memcpy(retrun_buffer, wav_info_header, 16);
	memcpy(retrun_buffer + 16, result.c_str(), result.length());
	send(con_id_, retrun_buffer, result.length() + 16,0);
	delete buffer.data_;
}

bool SRSocketClient::is_ok()
{
	return running_;
}


/**********************************************************************************/

SRSocketSever::SRSocketSever(int cnt,int port)
	:client_count_(cnt),port_(port)
{
	
}

SRSocketSever::~SRSocketSever()
{

}

void SRSocketSever::start()
{
	int  slisten, sClient;
    struct sockaddr_in  servaddr;


    if( (slisten = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return ;
    }
    printf("----init socket----\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_);

	//绑定IP和端口  
	if( bind(slisten, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return ;
    }
    printf("----bind sucess----\n");
	//开始监听  
	if (listen(slisten, 5) == -1)
	{
		printf("listen error !");
		return ;
	}

	//循环接收数据  
	//SOCKET sClient;
	sockaddr_in remoteAddr;
	socklen_t nAddrlen = sizeof(remoteAddr);
	while (true)
	{
		printf("等待连接...\n");
		sClient = accept(slisten, (struct sockaddr*)&remoteAddr, &nAddrlen);
		if (sClient == -1)
		{
			printf("accept error !");
			continue;
		}
		for (int i = 0; i < clients_.size(); i++)
		{
			if (!clients_[i]->is_ok())
			{
				delete clients_[i];
				clients_.erase(clients_.begin() + i);
			}
		}
		if (clients_.size() < client_count_)
		{
		
			printf("接收到一个连接：%d \r\n", (int)sClient);
			SRSocketClient* client = new SRSocketClient(sClient);
			clients_.push_back(client);
			client->run();
		}
		else
		{
			printf("连接达到上限拒绝连接：%d \r\n", (int)sClient);
			close(sClient);
		}
	}
}

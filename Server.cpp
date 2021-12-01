#include "thread.h"
#include "socketserver.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include "Semaphore.h"
#include <list>
#include <vector>
#include <thread>

using namespace Sync;


class SocketThread : public Thread
{
private:
    //Soket Reference
    Socket &socket;
    ByteArray data;
	//Indicates no. chat room
	int roomNum;
	int port;
    //Deteermines to terminate or not to terminate
    bool& terminate;

    //SocketThread pointers
    std::vector<SocketThread*> &socketThreadsHolder;


public:
	SocketThread(Socket& socket, std::vector<SocketThread*> &clientSockThr, bool &terminate, int port) :
		socket(socket), socketThreadsHolder(clientSockThr), terminate(terminate), port(port)
	{}

    ~SocketThread(){
		this->terminationEvent.Wait();
	}

    Socket& GetSocket(){
        return socket;
    }

    const int GetRoomNum(){
        return roomNum;
    }

    virtual long ThreadMain(){
		//convert port number to string
		std::string portNum = std::to_string(port);
		//generated semaphore for each socket thread, referencing the port number retrived above
		Semaphore clientBlock(portNum);

		try {
			//get bytestream data
			socket.Read(data);
			//convert bytestream to string
			std::string chatRoomNo = data.ToString();
			chatRoomNo = chatRoomNo.substr(1, chatRoomNo.size() - 1);
			roomNum = std::stoi(chatRoomNo);
			std::cout << "Current chat room no.: " << roomNum << std::endl;

			//runs until thread is terminated
			while(!terminate) {
				int socketResult = socket.Read(data);
				//if the client closes the socket, terminate the associated socket thread
				if (socketResult == 0)	break;

				std::string recv = data.ToString();
				if(recv == "shutdown\n") {
					//mutual exclusion assurance
					clientBlock.Wait();
					//iterator method to select and erase the socket thread
					socketThreadsHolder.erase(std::remove(socketThreadsHolder.begin(), socketThreadsHolder.end(), this), socketThreadsHolder.end());
					clientBlock.Signal();

					std::cout<< "A client is leaving the server. Erase Client!" << std::endl;
					break;
				}

				//check if a slash is appended as the first character so we can remove it
				if (recv[0] == '/') {
					//gets the chat room num
					std::string stringChat = recv.substr(1, recv.size() - 1);
					roomNum = std::stoi(stringChat);
					std::cout << "A client just joined room! " << roomNum << std::endl;
					continue;
				}

				clientBlock.Wait();
				//iterates over all the sockets
				for (int i = 0; i < socketThreadsHolder.size(); i++) {
					SocketThread *clientSocketThread = socketThreadsHolder[i];
					//confirms if the clients are in the same room then allows for message sending
					if (clientSocketThread->GetRoomNum() == roomNum)
					{
						Socket &clientSocket = clientSocketThread->GetSocket();
						ByteArray sendBa(recv);
						clientSocket.Write(sendBa);
					}
				}
				clientBlock.Signal();
			}
		}
		//catch string exceptions
		catch(std::string &s) {
			std::cout << s << std::endl;
		}
		//catch thrown exceptions and make it easy to identify
		catch(std::exception &e){
			std::cout << "A client has quit the app!" << std::endl;
		}
		std::cout << "A client has left!" << std::endl;

		return 0;
	}
};

class ServerThread : public Thread
{
private:
	//socket reference
    SocketServer &server;
	//number of chat rooms
	int  numOfRooms;
	int port;
    std::vector<SocketThread*> socketThrHolder;

    //Deteermines to terminate or not to terminate
    bool terminate = false;

public:
    ServerThread(SocketServer& server, int  numOfRooms, int port)
    : server(server), numOfRooms(numOfRooms), port(port)
    {}

    ~ServerThread()
    {
		//loops over client threads when exiting
        for (auto thread : socketThrHolder)
        {
            try
            {
                //close the socket
                Socket& toClose = thread->GetSocket();
                toClose.Close();
            }
            catch (...)
            {
                //This will catch all exceptions
            }
        }
		std::vector<SocketThread*>().swap(socketThrHolder);
        terminate = true;
    }
	
    virtual long ThreadMain()
    {
        while (true)
        {
            try {
				//convert port number to string
                std::string portNum = std::to_string(port);
                Semaphore serverBlock(portNum, 1, true);
				//receives total number of chats through socket
                std::string allChats = std::to_string(numOfRooms) + '\n';
				//converts no. of chats to ByteArray
                ByteArray allChats_conv(allChats);
                Socket sock = server.Accept();
				//send number of total chats
                sock.Write(allChats_conv);
				//initialize new socket
                Socket* newConnection = new Socket(sock);
                Socket &socketReference = *newConnection;
                socketThrHolder.push_back(new SocketThread(socketReference, std::ref(socketThrHolder), terminate, port));
            }
			//catch string exceptions
            catch (std::string error)
            {
                std::cout << "error:" << error << std::endl;
				//exit thread
                return 1;
            }
			//in case of unexpected shutdown
			catch (TerminationException terminationException)
			{
				std::cout << "The Server has shut down!" << std::endl;
				//exit and throw exception
				return terminationException;
			}
        }
    }
};

int main(void) {
	//port initializer
    int port = 3005;
	//max rooms allowed for server(can be adjusted)
    int rooms = 10;

    std::cout << "I am Server" << std::endl
		<<"~ enter 'done' to quit" << std::endl;

	//initialize server
    SocketServer server(port);
	//initialize thread
    ServerThread st(server, rooms, port);
	//shutdown handlers
	FlexWait cinWaiter(1, stdin);
	cinWaiter.Wait();
	std::cin.get();
	server.Shutdown();
    std::cout << "Gracefully Terminated!" << std::endl;
}

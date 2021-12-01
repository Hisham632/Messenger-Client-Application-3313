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
	//Indicates no. of chat rooms
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

    ///SocketThread pointers
    std::vector<SocketThread*> socketThrHolder;

	// Given socket port number.
	int port;

	// Given chats number.
	int numberRooms;

	// Flag for termination.
    bool terminate = false;

	/* Alternate Version Using Delete, but requires ArrayList encapsulation.
	Socket **socketsHolder = new Socket*[10];
	SocketThread **sckThreadsHolder = new SocketThread*[10];*/

public:
    ServerThread(SocketServer& server, int numberRooms, int port)
    : server(server), numberRooms(numberRooms), port(port)
    {}

    ~ServerThread()
    {
		// Cleanup
		//this->terminationEvent.Wait();

		// MAKE SURE TO CLOSE ALL SOCKETS (assigning null / deallocating memory).
		//std::vector<Socket*>().swap(socketsHolder);
		//std::vector<SocketThread*>().swap(sckThreadsHolder);

		// delete[] socketsHolder;
		// delete[] sckThreadsHolder;


        // Close the client sockets.
        for (auto thread : socketThrHolder)
        {
            try
            {
                // Close the socket.
                Socket& toClose = thread->GetSocket();
                toClose.Close();
            }
            catch (...)
            {
                // This will catch all exceptions.
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
				// Stringify port number.
                std::string stringPortNum = std::to_string(port);
                std::cout << "FlexWait/Natural blocking call on client!" <<std::endl;

				// Main owner semaphore to block other semaphores by name.
                Semaphore serverBlock(stringPortNum, 1, true);

				// Front-end receives number of chats through socket.
                std::string allChats = std::to_string(numberRooms) + '\n';

				// Byte array conversion for number of chats.
                ByteArray allChats_conv(allChats);


                // Wait for a client socket connection
                Socket sock = server.Accept();

				// Send number of chats.
                sock.Write(allChats_conv);
                Socket* newConnection = new Socket(sock);

				//socketsHolder.push_back(newConnection);

                // Pass a reference to this pointer into a new socket thread.
                Socket &socketReference = *newConnection;
                socketThrHolder.push_back(new SocketThread(socketReference, std::ref(socketThrHolder), terminate, port));
            }
			// Catch string-thrown exception.
            catch (std::string error)
            {
                std::cout << "ERROR: " << error << std::endl;
				// Exit thread function.
                return 1;
            }
			// In case of unexpected shutdown.
			catch (TerminationException terminationException)
			{
				std::cout << "Server has shut down!" << std::endl;
				// Exit with exception thrown.
				return terminationException;
			}
        }
    }
};

int main(void) {
	// AWS port.
    int port = 3005;

	// Admin sets value of number of chat rooms for the server.
    int rooms = 20;

    std::cout << "SE 3313 Server" << std::endl
		<<"Type done to quit the server..." << std::endl;

	// Create our server.
    SocketServer server(port);

	// Need a thread to perform sever operations.
    ServerThread st(server, rooms, port);

	// This will wait for input to shutdown the server
	FlexWait cinWaiter(1, stdin);
	cinWaiter.Wait();
	std::cin.get();

	// Cleanup, including exiting clients, when the user presses enter

	// Shut down and clean up the server
	server.Shutdown();

    std::cout << "Good-bye!" << std::endl;
}


#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;

// This thread handles the connection to the server
class ClientThread : public Thread
{
private:
	// Reference to our connected socket
	Socket& socket;

	// Data to send to server
	ByteArray data;
	std::string data_str;
public:
	ClientThread(Socket& socket)
	: socket(socket)
	{}

	~ClientThread()
	{}

	virtual long ThreadMain()
	{
		std::cout << "Type 'done' to exit: ";
		std::cout.flush();

		//get data from the byte array
		std::getline(std::cin, data_str);
		data = ByteArray(data_str);

		// Write to the server
		socket.Write(data);

		// Get the response
		socket.Read(data);
		data_str = data.ToString();
		std::cout << "Server Response: " << data_str << std::endl;
		return 0;
	}
};

int main(void)
{
	// Create socket
	Socket socket("127.0.0.1", 3005);
	ClientThread clientThread(socket);
	while(1)
	{
		sleep(1);
	}
	socket.Close();

	return 0;
}

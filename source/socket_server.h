#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

// Forward declare to avoid pulling winsock into every TU
struct SocketServerImpl;

struct PendingCommand
{
	std::string requestJson;
	int         clientSocket;  // socket fd to send response back on
};

class SocketServer
{
public:
	SocketServer();
	~SocketServer();

	bool Start(int port = 5555);
	void Stop();
	bool IsRunning() const;

	bool HasPendingCommand() const;
	bool PopCommand(PendingCommand& out);
	void SendResponse(int clientSocket, const std::string& responseJson);

private:
	void ListenerThread(int port);
	void HandleClient(int clientSocket);

	std::atomic<bool>          _running{false};
	std::thread                _listenerThread;

	mutable std::mutex         _queueMutex;
	std::queue<PendingCommand> _commandQueue;

	mutable std::mutex         _sendMutex;

	int                        _listenSocket = -1;
};

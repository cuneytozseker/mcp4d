#include "socket_server.h"

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
	typedef int socklen_t;
	#define CLOSE_SOCKET closesocket
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#define CLOSE_SOCKET close
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
#endif

#include <cstring>

static bool g_wsaInitialized = false;

static bool InitWSA()
{
#ifdef _WIN32
	if (!g_wsaInitialized)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return false;
		g_wsaInitialized = true;
	}
#endif
	return true;
}

static void CleanupWSA()
{
#ifdef _WIN32
	if (g_wsaInitialized)
	{
		WSACleanup();
		g_wsaInitialized = false;
	}
#endif
}

SocketServer::SocketServer()  = default;
SocketServer::~SocketServer() { Stop(); }

bool SocketServer::Start(int port)
{
	if (_running.load())
		return true;

	if (!InitWSA())
		return false;

	_running.store(true);
	_listenerThread = std::thread(&SocketServer::ListenerThread, this, port);
	return true;
}

void SocketServer::Stop()
{
	if (!_running.load())
		return;

	_running.store(false);

	// Close the listen socket to unblock accept()
	if (_listenSocket != (int)INVALID_SOCKET)
	{
		shutdown(_listenSocket, 2); // SD_BOTH
		CLOSE_SOCKET(_listenSocket);
		_listenSocket = (int)INVALID_SOCKET;
	}

	if (_listenerThread.joinable())
		_listenerThread.join();

	// Close any pending client sockets that never got a response
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		while (!_commandQueue.empty())
		{
			PendingCommand& cmd = _commandQueue.front();
			if (cmd.clientSocket != (int)INVALID_SOCKET)
				CLOSE_SOCKET(cmd.clientSocket);
			_commandQueue.pop();
		}
	}

	CleanupWSA();
}

bool SocketServer::IsRunning() const
{
	return _running.load();
}

bool SocketServer::HasPendingCommand() const
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	return !_commandQueue.empty();
}

bool SocketServer::PopCommand(PendingCommand& out)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	if (_commandQueue.empty())
		return false;
	out = std::move(_commandQueue.front());
	_commandQueue.pop();
	return true;
}

void SocketServer::SendResponse(int clientSocket, const std::string& responseJson)
{
	std::lock_guard<std::mutex> lock(_sendMutex);

	std::string msg = responseJson + "\n";
	const char* data = msg.c_str();
	int remaining = (int)msg.size();

	while (remaining > 0)
	{
		int sent = send(clientSocket, data, remaining, 0);
		if (sent <= 0)
			break;
		data += sent;
		remaining -= sent;
	}

	shutdown(clientSocket, 2);
	CLOSE_SOCKET(clientSocket);
}

void SocketServer::ListenerThread(int port)
{
	_listenSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_listenSocket == (int)INVALID_SOCKET)
	{
		_running.store(false);
		return;
	}

	// Allow port reuse
	int opt = 1;
	setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons((u_short)port);

	if (bind(_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		CLOSE_SOCKET(_listenSocket);
		_listenSocket = (int)INVALID_SOCKET;
		_running.store(false);
		return;
	}

	if (listen(_listenSocket, 5) == SOCKET_ERROR)
	{
		CLOSE_SOCKET(_listenSocket);
		_listenSocket = (int)INVALID_SOCKET;
		_running.store(false);
		return;
	}

	while (_running.load())
	{
		// Use select() with a 500ms timeout so we can check _running periodically
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET((SOCKET)_listenSocket, &readSet);

		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000; // 500ms

		int selResult = select(_listenSocket + 1, &readSet, nullptr, nullptr, &tv);
		if (selResult <= 0)
			continue; // timeout or error — loop back and check _running

		sockaddr_in clientAddr{};
		socklen_t clientLen = sizeof(clientAddr);
		int clientSocket = (int)accept(_listenSocket, (sockaddr*)&clientAddr, &clientLen);

		if (clientSocket == (int)INVALID_SOCKET)
			break;

		HandleClient(clientSocket);
	}

	_running.store(false);
}

void SocketServer::HandleClient(int clientSocket)
{
	// Set a recv timeout so we don't block the listener thread forever
#ifdef _WIN32
	DWORD timeout = 5000; // 5 seconds
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif

	std::string buffer;
	char chunk[4096];

	while (true)
	{
		int received = recv(clientSocket, chunk, sizeof(chunk) - 1, 0);
		if (received <= 0)
			break;

		chunk[received] = '\0';
		buffer.append(chunk, received);

		// Check for newline delimiter — one command per line
		auto nlPos = buffer.find('\n');
		if (nlPos != std::string::npos)
		{
			std::string line = buffer.substr(0, nlPos);

			PendingCommand cmd;
			cmd.requestJson = std::move(line);
			cmd.clientSocket = clientSocket;

			std::lock_guard<std::mutex> lock(_queueMutex);
			_commandQueue.push(std::move(cmd));
			return; // Don't close socket — response will be sent later
		}
	}

	// Client disconnected without newline — try to use whatever we got
	if (!buffer.empty())
	{
		PendingCommand cmd;
		cmd.requestJson = std::move(buffer);
		cmd.clientSocket = clientSocket;

		std::lock_guard<std::mutex> lock(_queueMutex);
		_commandQueue.push(std::move(cmd));
	}
	else
	{
		CLOSE_SOCKET(clientSocket);
	}
}

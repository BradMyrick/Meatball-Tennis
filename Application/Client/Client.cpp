// Client.cpp : handles all client network functions.
#include "Client.h"
#include "../NetworkMessage.h"
#include <WS2tcpip.h>

// Initializes the client; connects to the server.
int Client::init(char* address, uint16_t port, uint8_t _player)
{
	if (inet_addr(address) == INADDR_NONE)
		return ADDRESS_ERROR;

	state.player0.keyUp = state.player0.keyDown = true;
	state.player1.keyUp = state.player1.keyDown = true;
	state.gamePhase = WAITING;
	// TODO:
	//       1) Set the player.
	player = (int)_player;
	//       2) Set up the connection.
	clSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clSocket == INVALID_SOCKET)
		return SETUP_ERROR;
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = inet_addr(address);
	//       3) Connect to the server.
	int result = connect(clSocket, (sockaddr*)&saddr, sizeof(sockaddr));
	if (result == SOCKET_ERROR)
		return DISCONNECT;

	NetworkMessage msg(IO::_OUTPUT);
	msg.writeByte(1); //padding
	msg.writeByte(1); //get rid of this extra padding later
	msg.writeByte(CL_CONNECT);
	msg.writeByte(player);
	sendNetMessage(clSocket, msg);

	//       4) Get response from server.
	NetworkMessage rmsg(IO::_INPUT);
	recvNetMessage(clSocket, rmsg);
	sequence = rmsg.readShort();
	//       5) Make sure to mark the client as running.
	return SUCCESS;
}

// Receive and process messages from the server.
int Client::run()
{
	// TODO: Continuously process messages from the server aslong as the client running.
	// HINT: Set game phase to DISCONNECTED on SV_CL_CLOSE! (Try calling stop().)
	// HINT: You can keep track of the number of snapshots with a static variable...
	return DISCONNECT;
}

// Clean up and shut down the client.
void Client::stop()
{
	// TODO:
	//       1) Make sure to send a SV_CL_CLOSE message.
	//       2) Make sure to mark the client as shutting down and close socket.
	//       3) Set the game phase to DISCONNECTED.
}

// Send the player's input to the server.
int Client::sendInput(int8_t keyUp, int8_t keyDown, int8_t keyQuit)
{
	if (keyQuit)
	{
		stop();
		return SHUTDOWN;
	}

	cs.enter();
	if (player == 0)
	{
		state.player0.keyUp = keyUp;
		state.player0.keyDown = keyDown;
	}
	else
	{
		state.player1.keyUp = keyUp;
		state.player1.keyDown = keyDown;
	}
	cs.leave();

	//TODO:	Transmit the player's input status.

	return SUCCESS;
}

// Copies the current state into the struct pointed to by target.
void Client::getState(GameState* target)
{
	// TODO: Copy state into target.

}

// Sends a SV_CL_CLOSE message to the server (private, suggested)
void Client::sendClose()
{
	// TODO: Send a CL_CLOSE message to the server.

}

// Sends a CL_ALIVE message to the server (private, suggested)
int Client::sendAlive()
{
	// TODO: Send a CL_ALIVE message to the server.

	return SUCCESS;
}

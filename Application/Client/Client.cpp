// Client.cpp : handles all client network functions.
#include "Client.h"
#include "../NetworkMessage.h"
#include <stdlib.h>
#include <WS2tcpip.h>

// Initializes the client; connects to the server.
int Client::snapCount = 0;
int Client::init(char* address, uint16_t port, uint8_t _player)
{
	if (inet_addr(address) == INADDR_NONE)
		return ADDRESS_ERROR;

	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;
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
	msg.writeByte(CL_CONNECT);
	msg.writeByte(player);
	sendNetMessage(clSocket, msg);

	//       4) Get response from server.
	NetworkMessage rmsg(IO::_INPUT);
	recvNetMessage(clSocket, rmsg);
	sequence = rmsg.readShort();
	//       5) Make sure to mark the client as running.
	if (rmsg.readByte() == SV_OKAY) {
		state.gamePhase = RUNNING;
		active = true;
		return SUCCESS;
	}
	else return SHUTDOWN;
}

// Receive and process messages from the server.
//int Client::run()
//{
//	snapCount = 0;
//	NetworkMessage msg(IO::_INPUT);
//	while (active) {
//		int result = recvNetMessage(clSocket, msg);
//		short tmpSeq = msg.readShort();
//		if (tmpSeq > sequence) {
//			sequence = tmpSeq;
//			if (result > 0) {
//				snapCount++;
//
//				if (snapCount >= 10)
//					sendAlive();
//
//				switch (msg.readByte()) {
//				case SV_SNAPSHOT:
//					state.gamePhase = msg.readByte();	//	Phase
//					state.ballX = msg.readShort();	//	BallX
//					state.ballY = msg.readShort();	//	BallY
//					state.player0.y = msg.readShort();	//	P0 - Y
//					state.player0.score = msg.readShort();	//	P0 - Score
//					state.player1.y = msg.readShort();	//	P1 - Y
//					state.player1.score = msg.readShort();	//	P1 - Score
//					break;
//
//				case SV_CL_CLOSE:
//					return SHUTDOWN;
//				}
//			}
//			else
//				return MESSAGE_ERROR;
//		}
//	}
//	return true;
//}

int Client::run()
{
	snapCount = 0;
	NetworkMessage msgIN(_INPUT);
	while (state.gamePhase == RUNNING)
	{
		int recvError = recvNetMessage(clSocket, msgIN);
		if (recvError == SOCKET_ERROR)
		{
			return CONNECT_ERROR;
		}
		uint16_t RecvseqNum = msgIN.readShort();
		if (sequence < RecvseqNum)
		{
			sequence = RecvseqNum;
			uint8_t tag = msgIN.readByte();
			if (tag == SV_CL_CLOSE)
			{
				stop();
			}
			else if (tag == SV_SNAPSHOT)
			{
				state.gamePhase = msgIN.readByte();
				state.ballX = msgIN.readShort();
				state.ballY = msgIN.readShort();
				state.player0.y = msgIN.readShort();
				state.player0.score = msgIN.readShort();
				state.player1.y = msgIN.readShort();
				state.player1.score = msgIN.readShort();
				snapCount++;
				if (snapCount >= 10)
				{
					NetworkMessage tmpOUT(_OUTPUT);
					tmpOUT.writeByte(CL_ALIVE);
					int sendResults = sendNetMessage(clSocket, tmpOUT);
					snapCount = 0;
				}
				if (state.gamePhase == GAMEOVER)
				{
					int num = 0;
					num = num + 1;

				}
			}
		}
	}
	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
			stop();
			break;
	}
	return DISCONNECT;
}

// Clean up and shut down the client.
void Client::stop()
{
	// TODO:
	//       1) Make sure to send a SV_CL_CLOSE message.
	sendClose();
	//       2) Make sure to mark the client as shutting down and close socket.
	active = false;
	closesocket(clSocket);
	//       3) Set the game phase to DISCONNECTED.
	state.gamePhase = DISCONNECTED;
}

// Send the player's input to the server.
int Client::sendInput(int8_t keyUp, int8_t keyDown, int8_t keyQuit)
{
	if (keyQuit)
	{
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
	NetworkMessage msg(IO::_OUTPUT);
	msg.writeByte(CL_KEYS);
	msg.writeByte(keyUp);
	msg.writeByte(keyDown);
	sendNetMessage(clSocket, msg);
	return SUCCESS;
}

// Copies the current state into the struct pointed to by target.
void Client::getState(GameState* target)
{
	// TODO: Copy state into target.
	target->ballX = state.ballX;
	target->ballY = state.ballY;
	target->gamePhase = state.gamePhase;
	target->player0 = state.player0;
	target->player1 = state.player1;

}

// Sends a SV_CL_CLOSE message to the server (private, suggested)
void Client::sendClose()
{
	// TODO: Send a CL_CLOSE message to the server.
	NetworkMessage msg(IO::_OUTPUT);
	msg.writeByte(SV_CL_CLOSE);
	sendNetMessage(clSocket, msg);
}

// Sends a CL_ALIVE message to the server (private, suggested)
int Client::sendAlive()
{
	// TODO: Send a CL_ALIVE message to the server.
	NetworkMessage msg(IO::_OUTPUT);
	msg.writeByte(CL_ALIVE);
	return sendNetMessage(clSocket, msg);
}

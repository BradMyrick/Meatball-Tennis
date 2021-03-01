// Server.cpp : Contains all functions of the server.
#include "Server.h"

// Initializes the server. (NOTE: Does not wait for player connections!)
int Server::init(uint16_t port) {
	initState();

	//	1) Set up a socket for listening.
	svSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (svSocket == INVALID_SOCKET)
		return SETUP_ERROR;

	memset((char*)&playerAddress[0], 0, sizeof(playerAddress[0]));//
	memset((char*)&playerAddress[1], 0, sizeof(playerAddress[1]));// i don't know, what I was tying didn't work.  I found this but I still crashes
	playerAddress[0].sin_family = AF_INET;
	playerAddress[0].sin_port = htons(port);
	playerAddress[0].sin_addr.S_un.S_addr = INADDR_ANY;

	//	Bind Socket to port
	int result = bind(svSocket, (SOCKADDR*)&playerAddress[0], sizeof(playerAddress[0]));
	if (result == SOCKET_ERROR)
		return ADDRESS_ERROR;

	//	2) Mark the server as active.
	seq1 = 0;
	seq2 = 0;
	active = true;
	return SUCCESS;
}

// Updates the server; called each game "tick".
int Server::update() {
	//	1) Get player input and process it.
	NetworkMessage msg(IO::_INPUT);
	//	Allocate socket address to store the src of msgs
	sockaddr_in saddr;
	int result;
	if (result = recvfromNetMessage(svSocket, msg, &saddr) == SOCKET_ERROR)
		return DISCONNECT;
	parseMessage(saddr, msg);
	//	2) If any player's timer exceeds 50, "disconnect" the player.
	if (playerTimer[0] > 50)
		disconnectClient(0);
	if (playerTimer[1] > 50)
		disconnectClient(1);
	//	3) Update the state and send the current snapshot to each player.

	updateState();
	sendState();
	return SUCCESS;
}

// Stops the server.
void Server::stop() {
	//       1) Sends a "close" message to each client.
	sendClose();
	//       2) Shuts down the server gracefully (update method should exit with SHUTDOWN code.)
	shutdown(svSocket, SD_BOTH);
	close(svSocket);
}

// Parses a message and responds if necessary. (private, suggested)
int Server::parseMessage(sockaddr_in& source, NetworkMessage& message) {
	// TODO: Parse a message from client "source."
	
	return SUCCESS;
}

// Sends the "SV_OKAY" message to destination. (private, suggested)
int Server::sendOkay(sockaddr_in& destination) {
	NetworkMessage msg(IO::_OUTPUT);
	if (destination.sin_port == playerAddress[0].sin_port) {
		seq1++;
		msg.writeShort(seq1);
	}
	else if (destination.sin_port == playerAddress[1].sin_port) {
		seq2++;
		msg.writeShort(seq2);
	}
	msg.writeByte(SV_OKAY);
	sendtoNetMessage(svSocket, msg, &destination);
	return SUCCESS;
}

// Sends the "SV_FULL" message to destination. (private, suggested)
int Server::sendFull(sockaddr_in& destination) {
	NetworkMessage msg(IO::_OUTPUT);
	if (destination.sin_port == playerAddress[0].sin_port) {
		seq1++;
		msg.writeShort(seq1);
	}
	else if (destination.sin_port == playerAddress[1].sin_port) {
		seq2++;
		msg.writeShort(seq2);
	}
	msg.writeByte(SV_FULL);
	sendtoNetMessage(svSocket, msg, &destination);
	return SUCCESS;
}

// Sends the current snapshot to all players. (private, suggested)
int Server::sendState() {
	NetworkMessage msg = (IO::_OUTPUT);;
	sendtoNetMessage(svSocket, msg, &playerAddress[0]);
	sendtoNetMessage(svSocket, msg, &playerAddress[1]);
	return SUCCESS;
}

// Sends the "SV_CL_CLOSE" message to all clients. (private, suggested)
void Server::sendClose() {
	NetworkMessage msg0(IO::_OUTPUT);
	NetworkMessage msg1(IO::_OUTPUT);

	seq1++;
	msg0.writeShort(seq1);
	msg0.writeByte(SV_CL_CLOSE);

	seq2++;
	msg1.writeShort(seq2);
	msg1.writeByte(SV_CL_CLOSE);

	sendtoNetMessage(svSocket, msg0, &playerAddress[0]);
	sendtoNetMessage(svSocket, msg1, &playerAddress[1]);
}

// Server message-sending helper method. (private, suggested)
int Server::sendMessage(sockaddr_in& destination, NetworkMessage& message) {
	// TODO: Send the message in the buffer to the destination.s
	return SUCCESS;
}

// Marks a client as connected and adjusts the game state.
void Server::connectClient(int player, sockaddr_in& source)
{
	playerAddress[player] = source;
	playerTimer[player] = 0;
	numUsers++;
	if (numUsers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = RUNNING;
}

// Marks a client as disconnected and adjusts the game state.
void Server::disconnectClient(int player)
{
	playerAddress[player].sin_addr.s_addr = INADDR_NONE;
	playerAddress[player].sin_port = 0;

	numUsers--;
	if (numUsers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = DISCONNECTED;
}

// Updates the state of the game.
void Server::updateState()
{
	// Tick counter.
	static int timer = 0;

	// Update the tick counter.
	timer++;

	// Next, update the game state.
	if (state.gamePhase == RUNNING)
	{
		// Update the player tick counters (for ALIVE messages.)
		playerTimer[0]++;
		playerTimer[1]++;

		// Update the positions of the player paddles
		if (state.player0.keyUp)
			state.player0.y--;
		if (state.player0.keyDown)
			state.player0.y++;

		if (state.player1.keyUp)
			state.player1.y--;
		if (state.player1.keyDown)
			state.player1.y++;

		// Make sure the paddle new positions are within the bounds.
		if (state.player0.y < 0)
			state.player0.y = 0;
		else if (state.player0.y > FIELDY - PADDLEY)
			state.player0.y = FIELDY - PADDLEY;

		if (state.player1.y < 0)
			state.player1.y = 0;
		else if (state.player1.y > FIELDY - PADDLEY)
			state.player1.y = FIELDY - PADDLEY;

		//just in case it get stuck...
		if (ballVecX)
			storedBallVecX = ballVecX;
		else
			ballVecX = storedBallVecX;

		if (ballVecY)
			storedBallVecY = ballVecY;
		else
			ballVecY = storedBallVecY;

		state.ballX += ballVecX;
		state.ballY += ballVecY;

		// Check for paddle collisions & scoring
		if (state.ballX < PADDLEX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player0.y && state.ballY < state.player0.y + PADDLEY)
			{
				state.ballX = PADDLEX;
				ballVecX *= -1;
			}
			// Otherwise, the second player has scored.
			else
			{
				newBall();
				state.player1.score++;
				ballVecX *= -1;
			}
		}
		else if (state.ballX >= FIELDX - PADDLEX - BALLX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player1.y && state.ballY < state.player1.y + PADDLEY)
			{
				state.ballX = FIELDX - PADDLEX - BALLX - 1;
				ballVecX *= -1;
			}
			// Otherwise, the first player has scored.
			else
			{
				newBall();
				state.player0.score++;
				ballVecX *= -1;
			}
		}

		// Check for Y position "bounce"
		if (state.ballY < 0)
		{
			state.ballY = 0;
			ballVecY *= -1;
		}
		else if (state.ballY >= FIELDY - BALLY)
		{
			state.ballY = FIELDY - BALLY - 1;
			ballVecY *= -1;
		}
	}

	// If the game is over...
	if ((state.player0.score > 10 || state.player1.score > 10) && state.gamePhase == RUNNING)
	{
		state.gamePhase = GAMEOVER;
		timer = 0;
	}
	if (state.gamePhase == GAMEOVER)
	{
		if (timer > 30)
		{
			initState();
			state.gamePhase = RUNNING;
		}
	}
}

// Initializes the state of the game.
void Server::initState()
{
	playerAddress[0].sin_addr.s_addr = INADDR_NONE;
	playerAddress[1].sin_addr.s_addr = INADDR_NONE;
	playerTimer[0] = playerTimer[1] = 0;
	numUsers = 0;

	state.gamePhase = DISCONNECTED;

	state.player0.y = 0;
	state.player1.y = FIELDY - PADDLEY - 1;
	state.player0.score = state.player1.score = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;

	newBall(); // Get new random ball position

	// Get a new random ball vector that is reasonable
	ballVecX = (rand() % 10) - 5;
	if ((ballVecX >= 0) && (ballVecX < 2))
		ballVecX = 2;
	if ((ballVecX < 0) && (ballVecX > -2))
		ballVecX = -2;

	ballVecY = (rand() % 10) - 5;
	if ((ballVecY >= 0) && (ballVecY < 2))
		ballVecY = 2;
	if ((ballVecY < 0) && (ballVecY > -2))
		ballVecY = -2;
}

// Places the ball randomly within the middle half of the field.
void Server::newBall()
{
	// (randomly in 1/2 playable area) + (1/4 playable area) + (left buffer) + (half ball)
	state.ballX = (rand() % ((FIELDX - 2 * PADDLEX - BALLX) / 2)) +
		((FIELDX - 2 * PADDLEX - BALLX) / 4) + PADDLEX + (BALLX / 2);

	// (randomly in 1/2 playable area) + (1/4 playable area) + (half ball)
	state.ballY = (rand() % ((FIELDY - BALLY) / 2)) + ((FIELDY - BALLY) / 4) + (BALLY / 2);
}

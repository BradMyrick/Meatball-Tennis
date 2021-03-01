#include "../platform.h"
#include "../definitions.h"
#include "../NetworkMessage.h"
class Server
{
private:
	bool active;
	SOCKET svSocket;
	sockaddr_in playerAddress[2];

	int numUsers, seq1, seq2;
	int playerTimer[2];

	int storedBallVecX, storedBallVecY;
	int ballVecX, ballVecY;
	GameState state;

public:
	inline Server() : active(false), svSocket(INVALID_SOCKET) { }
	int init(uint16_t port);
	int update();
	void stop();

private:
	int parseMessage(sockaddr_in& source, NetworkMessage& message);

	int sendOkay(sockaddr_in& destination);
	int sendFull(sockaddr_in& destination);
	int sendState();
	void sendClose();

	int sendMessage(sockaddr_in& destination, NetworkMessage& message);

	void initState();
	void updateState();
	void connectClient(int player, sockaddr_in& source);
	void disconnectClient(int player);
	void newBall();
};

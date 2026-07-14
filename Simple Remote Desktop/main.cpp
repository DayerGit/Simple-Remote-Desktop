#include "Client.h"
#include "Server.h"

int main() {
#ifdef CLIENT
	Client();
#else
	Server();
#endif
	return 0;
}

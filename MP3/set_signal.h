#include <csignal>
#include <cstddef>
void set_signal(int signo, void (*f)(int signo))
{
	struct sigaction sa;
	sa.sa_handler = f;
	sa.sa_flags = SA_INTERRUPT;
	sigaction(signo, &sa, NULL);
}
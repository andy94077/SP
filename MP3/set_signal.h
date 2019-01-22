#include <csignal>
#include <cstddef>
void set_signal(int signo, void (*f)(int signo))
{
	struct sigaction sa;
	sa.sa_handler = f;
	sigaction(signo, &sa, NULL);
}
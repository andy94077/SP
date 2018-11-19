#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <cstdlib>
#include <cstring>

#include "loser_peer.h"


int init_config(Config& conf, char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    /* TODO parse the config file */

    close(fd);
    return 0;
}




int main(int argc, char *argv[])
{
    /* Make sure argv has a config path */
    assert(argc == 2);
    int ret;

    /* Load config file */
    Config config(argv[1]);
    if(config.fail())
        exit(EXIT_FAILURE);

    /* Create host UNIX socket */
    sockaddr_un sock_addr;
    int connection_socket;

    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket < 0)
        exit(EXIT_FAILURE);

    memset(&sock_addr, 0, sizeof(sockaddr_un));

    /* Bind socket to a name. */
    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, config.user_name.c_str());

    ret = bind(connection_socket, (const sockaddr *) &sock_addr, sizeof(sockaddr_un));
    if (ret == -1)
        exit(EXIT_FAILURE);

    /* Prepare for accepting connections */
    ret = listen(connection_socket, 20);
    if (ret == -1)
        exit(EXIT_FAILURE);


    /* Enter the serving loop.
     * It calls select() to check if any file descriptor is ready.
     * You may look up the manpage select(2) for details.
     */

    const int max_fd = sysconf(_SC_OPEN_MAX);

    fd_set read_set;
    fd_set write_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    FD_SET(STDIN_FILENO, &read_set);       /* check for user input */
    FD_SET(connection_socket, &read_set);  /* check for new peer connections */

    while (true)
    {
        timeval tv;
        fd_set working_read_set, working_write_set;

        memcpy(&working_read_set, &read_set, sizeof(working_read_set));
        memcpy(&working_write_set, &write_set, sizeof(working_write_set));

        ret = select(max_fd, &working_read_set, &working_write_set, NULL, NULL);

        //We assume it doesn't happen.
        if (ret < 0)   exit(EXIT_FAILURE);
        //No fd is ready
        if (ret == 0)    continue;

        if (FD_ISSET(STDIN_FILENO, &working_read_set))
        {
            /* TODO Handle user commands */
        }

        if (FD_ISSET(connection_socket, &working_read_set))
        {
            int peer_socket = accept(connection_socket, NULL, NULL);
            if (peer_socket < 0)
                exit(EXIT_FAILURE);

            /* TODO Store the peer fd */
        }
    }

    /* finalize */
    close(connection_socket);
    return 0;
}

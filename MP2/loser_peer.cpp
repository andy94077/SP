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
int create_listen_sd(const string& path)
{
    int listen_sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_sd < 0)
        exit(EXIT_FAILURE);

    sockaddr_un sock_addr;
    //Bind socket to a name.
    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, path.c_str());
    remove(sock_addr.sun_path);
    if (bind(listen_sd, (const sockaddr *)&sock_addr, sizeof(sockaddr_un)) < 0)
        exit(EXIT_FAILURE);

    // Prepare for accepting connections
    if(listen(listen_sd, 20)<0)
        exit(EXIT_FAILURE);
    return listen_sd;
}
int connect_peer_sd(const string& peer_path)
{
    int peer_sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(peer_sd<0)
        exit(EXIT_FAILURE);

    sockaddr_un socket_addr;
    socket_addr.sun_family = AF_UNIX;
    strcpy(socket_addr.sun_path, peer_path.c_str());
    return (connect(peer_sd, (const sockaddr *)&socket_addr, sizeof(sockaddr_un))>=0)?peer_sd:-1;
}
int main(int argc, char *argv[])
{
    // Make sure argv has a config path
    assert(argc == 2);

    // Load config file
    Config config(argv[1]);
    if(config.fail())
        exit(EXIT_FAILURE);

    cout << "## user_socket:" << config.user_socket << '\n';
    for (int i = 0; i < config.peer_n;i++)
        cout << "## peer_socket:" << config.peer_socket[i] << '\n';

    // Create host UNIX socket
    int listen_sd = create_listen_sd(config.user_socket);
    printf("## listen_sd:%d\n", listen_sd);

    fd_set read_set;
    FD_ZERO(&read_set);

    FD_SET(STDIN_FILENO, &read_set);       // check for user input 
    FD_SET(listen_sd, &read_set);  // check for new peer connections

    for (int i = 0; i < config.peer_n;i++)
    {
        int peer_sd = connect_peer_sd(config.peer_socket[i]);
        if(peer_sd>=0)
        {
            FD_SET(peer_sd, &read_set);
            cout << "connect " << config.peer_socket[i] << " by sd " << peer_sd << '\n';
        }
    }
    puts("ready");
    /* Enter the serving loop.
     * It calls select() to check if any file descriptor is ready.
     * You may look up the manpage select(2) for details.
     */
    const int max_fd = sysconf(_SC_OPEN_MAX);
    bool quit = false;
    while (!quit)
    {
        fd_set working_read_set;
        memcpy(&working_read_set, &read_set, sizeof(working_read_set));

        timeval tv;
        tv.tv_sec = tv.tv_usec = 0;
        int ret = select(max_fd, &working_read_set, NULL, NULL, NULL);

        //We assume it doesn't happen.
        if (ret < 0)   exit(EXIT_FAILURE);
        //No fd is ready
        //if (ret == 0)    continue;
        assert(ret > 0);

        for (int i = 0; i < max_fd;i++)
        {
            if(!FD_ISSET(i, &working_read_set)) continue;
            if (i == STDIN_FILENO)
            {
                // TODO Handle user commands 
                string line;
                getline(cin, line);
                if(line[0]=='q')
                    quit = true;
                cerr << "send:" << line << '\n';
                line.insert(0, config.user_socket + ": ");
                for (int j = 0; j < max_fd; j++)
                    if (FD_ISSET(j,&read_set) && j!=i && j!=listen_sd && send(j, line.data(), line.size(), 0) < 0)
                        cerr << "sd " << j << ": sending error\n";
            }
            else if (i == listen_sd)//new connection
            {
                int peer_sd = accept(listen_sd, NULL, NULL);
                if (peer_sd < 0)
                    exit(EXIT_FAILURE);

                //TODO Store the peer fd 
                FD_SET(peer_sd, &read_set);
                printf("## new connection on sd %d\n", peer_sd);
            }
            else//received data from other peers
            {
                char buf[256]={0};
                int bytes = recv(i, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0)
                {
                    cerr << "sd " << i << " offlines\n";
                    FD_CLR(i, &read_set);
                }
                else
                    printf("recv from sd %d, %d bytes:%s\n",i,bytes,buf);

            }
        }

    }

    // finalize 
    close(listen_sd);
    return 0;
}

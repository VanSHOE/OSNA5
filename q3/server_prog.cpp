#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include "../colors.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
#include <vector>
using namespace std;
/////////////////////////////

// Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x)                      \
    pthread_mutex_lock(&print_lock);  \
    cout << #x << " : " << x << endl; \
    pthread_mutex_unlock(&print_lock);
#define part                                               \
    pthread_mutex_lock(&print_lock);                       \
    cout << "-----------------------------------" << endl; \
    pthread_mutex_unlock(&print_lock);

///////////////////////////////
#define MAX_CLIENTS 100
#define PORT_ARG 8001

const int initial_msg_len = 256;

pthread_mutex_t print_lock;

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////
pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

///////////////////////////////

struct adjNode
{
    int dest;
    int delay;
    adjNode(int d, int w)
    {
        dest = d;
        delay = w;
    }
};

struct nodeView
{
    vector<vector<adjNode>> fullGraph;
    pthread_mutex_t lock;
};

string serializeGraph(vector<vector<adjNode>> &graph)
{
    string s = "";
    for (int i = 0; i < graph.size(); i++)
    {
        for (int j = 0; j < graph[i].size(); j++)
        {
            s += to_string(graph[i][j].dest) + " " + to_string(graph[i][j].delay) + " ";
        }
        s += ";";
    }
    return s;
}

vector<vector<adjNode>> deserializeGraph(string &s)
{
    vector<vector<adjNode>> graph;
    int i = 0;
    while (i < s.length())
    {
        vector<adjNode> temp;
        while (s[i] != ';')
        {
            int dest = 0, delay = 0;
            while (s[i] != ' ')
            {
                dest = dest * 10 + (s[i] - '0');
                i++;
            }
            i++;
            while (s[i] != ' ' && s[i] != ';')
            {
                delay = delay * 10 + (s[i] - '0');
                i++;
            }
            if (s[i] == ' ')
                i++;
            temp.pb(adjNode(dest, delay));
        }
        graph.pb(temp);
        i++;
    }
    return graph;
}

struct threadInfo
{
    int id;
    bool dirty;
    vector<adjNode> *neighbours;
    nodeView view;
};
void handle_client_connection(int client_socket_fd, threadInfo *me = NULL)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;

    while (true)
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        // debug(ret_val);
        if (ret_val <= 0)
        {
            // perror("Error read()");
            pthread_mutex_lock(&print_lock);
            printf("Server could not read msg sent from client\n");
            pthread_mutex_unlock(&print_lock);
            goto close_client_socket_ceremony;
        }

        // check if cmd starts from exit
        if (cmd.substr(0, 4) == "exit")
        {
            pthread_mutex_lock(&print_lock);
            cout << "Exit pressed on " << me->id << " with message: " << cmd << endl;
            pthread_mutex_unlock(&print_lock);
            goto close_client_socket_ceremony;
        }
        // deserialize command id|graph
        int cmd_id = 0;
        int i = 0;
        while (cmd[i] != '|')
        {
            cmd_id = cmd_id * 10 + (cmd[i] - '0');
            i++;
        }

        string graphS = cmd.substr(i + 1, cmd.length() - i - 1);

        vector<vector<adjNode>> graph = deserializeGraph(graphS);
        // print graph
        pthread_mutex_lock(&print_lock);

        yellow();
        cout << "Client sent to " << std::to_string(me->id) << ": " << cmd << "" << endl;
        reset();

        blue();
        cout << "Received graph from client " << me->id << " : " << endl;
        for (int i = 0; i < graph.size(); i++)
        {
            cout << "Node " << i << " : ";
            for (int j = 0; j < graph[i].size(); j++)
            {
                cout << "(" << graph[i][j].dest << " " << graph[i][j].delay << ") ";
            }
            cout << endl;
        }
        reset();

        // check if graph has anything new, then add to our own view
        // cout << "This happened\n";
        pthread_mutex_lock(&me->view.lock);
        // cout << "This happene2\n";
        int neighId = cmd_id;
        // check if it exists
        if (me->view.fullGraph.size() <= neighId)
        {
            me->view.fullGraph.resize(neighId + 1);
        }
        // check if it has anything new
        vector<vector<adjNode>> toAdd(graph.size());

        for (int nId = 0; nId < neighId + 1; nId++)
        {
            cout << "This happened: " << neighId << endl;

            for (int i = 0; i < graph[nId].size(); i++)
            {
                bool found = false;
                for (int j = 0; j < me->view.fullGraph[nId].size(); j++)
                {
                    if (me->view.fullGraph[nId][j].dest == graph[nId][i].dest && me->view.fullGraph[nId][j].delay == graph[nId][i].delay)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    toAdd[nId].pb(graph[nId][i]);
                }
            }
        }

        // add to our own view
        for (int i = 0; i < toAdd.size(); i++)
        {
            for (int j = 0; j < toAdd[i].size(); j++)
            {
                me->view.fullGraph[i].pb(toAdd[i][j]);
            }
        }
        // print all view
        bold();
        magenta();
        cout << "View of " << me->id << " after receiving from " << neighId << " : " << endl;
        for (int i = 0; i < me->view.fullGraph.size(); i++)
        {
            cout << "Node " << i << " : ";
            for (int j = 0; j < me->view.fullGraph[i].size(); j++)
            {
                cout << "(" << me->view.fullGraph[i][j].dest << " " << me->view.fullGraph[i][j].delay << ") ";
            }
            cout << endl;
        }
        reset();
        pthread_mutex_unlock(&me->view.lock);
        pthread_mutex_unlock(&print_lock);

        string msg_to_send_back = "Ack: " + cmd;

        ////////////////////////////////////////
        // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
        // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

        int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
        // debug(sent_to_client);
        if (sent_to_client == -1)
        {
            perror("Error while writing to client. Seems socket has been closed");
            goto close_client_socket_ceremony;
        }
    }

close_client_socket_ceremony:
    close(client_socket_fd);
    pthread_mutex_lock(&print_lock);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
    pthread_mutex_unlock(&print_lock);
    // return NULL;
}

void *threadListener(void *arg)
{
    struct threadInfo *info = (struct threadInfo *)arg;

    int i, j, k, t, n;

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }
    int opt = 1;
    if (setsockopt(wel_socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG + info->id + 1;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);

    pthread_mutex_lock(&print_lock);
    cout << "Server " << info->id << " has started listening on the LISTEN PORT" << endl;
    pthread_mutex_unlock(&print_lock);
    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
        of the server process. When the server “hears” the knocking, it creates a new door—
        more precisely, a new socket that is dedicated to that particular client.
        */
        // accept is a blocking call
        pthread_mutex_lock(&print_lock);
        printf("%d waiting for a new client to request for a connection\n", info->id);
        pthread_mutex_unlock(&print_lock);
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        pthread_mutex_lock(&print_lock);
        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        pthread_mutex_unlock(&print_lock);
        handle_client_connection(client_socket_fd, info);
    }
}

void *nodeThread(void *arg)
{
    threadInfo *me = (threadInfo *)arg;
    pthread_mutex_init(&me->view.lock, NULL);

    // add our own neighbours in fullgraph
    pthread_mutex_lock(&me->view.lock);
    // resize if size is less than me->id
    if (me->view.fullGraph.size() <= me->id)
    {
        me->view.fullGraph.resize(me->id + 1);
    }
    me->view.fullGraph[me->id] = *me->neighbours;
    pthread_mutex_unlock(&me->view.lock);

    pthread_t listener;
    pthread_create(&listener, NULL, threadListener, (void *)me);
    sleep(1);

    int myId = me->id;
    vector<adjNode> *neighbours = me->neighbours;

    // connect to other threads and send our current view
    for (int i = 0; i < neighbours->size(); i++)
    {
        int neighbourId = (*neighbours)[i].dest;
        int neighbourPort = (*neighbours)[i].dest + PORT_ARG + 1;

        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0)
        {
            perror("Error while creating socket");
            exit(-1);
        }

        struct sockaddr_in serv_addr_obj;
        bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
        serv_addr_obj.sin_family = AF_INET;
        serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
        serv_addr_obj.sin_port = htons(neighbourPort);

        if (connect(sock_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
        {
            string error = "Error while connecting to neighbour " + to_string(neighbourId) + " from " + to_string(myId);
            perror(error.c_str());
            continue;
        }

        // send hi
        pthread_mutex_lock(&me->view.lock);
        // string hi = "hi " + to_string(myId) + "\n";
        string graph2 = serializeGraph(me->view.fullGraph);
        string graph = "This is a graph" + std::to_string(graph2.length());
        pthread_mutex_unlock(&me->view.lock);
        int sent = send_string_on_socket(sock_fd, std::to_string(me->id) + "|" + graph2);

        if (sent < 0)
        {
            perror("Error while sending hi");
            exit(-1);
        }

        read_string_from_socket(sock_fd, BUFSIZ);

        sleep(2);

        // send exit
        string exitM = "exit " + to_string(myId) + "\n";
        sent = send_string_on_socket(sock_fd, exitM);
        if (sent < 0)
        {
            perror("Error while sending exit");
            exit(-1);
        }
    }

    // wait for join
    pthread_join(listener, NULL);
    // stopping
    pthread_mutex_lock(&print_lock);
    printf("Node %d is stopping\n", me->id);
    pthread_mutex_unlock(&print_lock);

    return NULL;
}

int main(int argc, char *argv[])
{
    int nodes, edges;
    pthread_mutex_init(&print_lock, NULL);

    cin >> nodes >> edges;
    vector<vector<adjNode>> adj_list(nodes);
    // start end delay adjacency_list

    for (int i = 0; i < edges; i++)
    {
        int start, end, delay;
        cin >> start >> end >> delay;
        adj_list[start].pb(adjNode(end, delay));
        adj_list[end].pb(adjNode(start, delay));
    }

    // create thread for each node and give it its neighbours
    pthread_t threads[nodes];

    for (int i = 0; i < nodes; i++)
    {
        threadInfo *t = new threadInfo;
        t->id = i;
        t->neighbours = &adj_list[i];
        t->dirty = true;
        int rc = pthread_create(&threads[i], NULL, nodeThread, (void *)t);

        if (rc)
        {
            pthread_mutex_lock(&print_lock);
            cout << "Error:unable to create thread," << rc << endl;
            pthread_mutex_unlock(&print_lock);
            exit(-1);
            exit(-1);
        }
    }

    for (int i = 0; i < nodes; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
    int i, j, k, t, n;

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    // reuse addr and port

    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    pthread_mutex_lock(&print_lock);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    pthread_mutex_unlock(&print_lock);
    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
        of the server process. When the server “hears” the knocking, it creates a new door—
        more precisely, a new socket that is dedicated to that particular client.
        */
        // accept is a blocking call
        pthread_mutex_lock(&print_lock);
        printf("Waiting for a new client to request for a connection\n");
        pthread_mutex_unlock(&print_lock);
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        pthread_mutex_lock(&print_lock);
        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        pthread_mutex_unlock(&print_lock);
        handle_client_connection(client_socket_fd);
    }

    // wait for join
    for (int i = 0; i < nodes; i++)
    {
        pthread_join(threads[i], NULL);
    }

    close(wel_socket_fd);
    return 0;
}
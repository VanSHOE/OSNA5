Using boilerplate code for making the client and server

- [Client](#client)
  - [Extra Functions](#extra-functions)
    - [get_socket_fd](#get_socket_fd)
    - [begin_process](#begin_process)
- [Server](#server)
  - [Extra Functions](#extra-functions-1)
    - [serializeGraph](#serializegraph)
    - [deserializeGraph](#deserializegraph)
    - [handle_client_connection](#handle_client_connection)
    - [handle_client_data_connection](#handle_client_data_connection)
    - [nodeThread](#nodethread)
  - [Algorithm](#algorithm)
    - [Layer 1](#layer-1)
    - [Layer 2](#layer-2)
- [How would I handle server failure?](#how-would-i-handle-server-failure)

## Common Functions

### read_string_from_socket

Responsible for reading a string from the socket. Returns the string and the number of bytes read.

### send_string_on_socket

Responsible for sending a string on the socket. Returns the number of bytes sent.

# Client

Basically a simple shell for communicating with the server

## Extra Functions

### get_socket_fd

Responsible for getting the socket file descriptor on which the client connects to the server.

### begin_process

Responsible for starting the process of the client. It calls the functions to get the socket file descriptor, read the string from the socket, and send the string on the socket to the server.

Note: When sending a send command. We add a -1 after two tokens just to accomodate the "source" which is useful later on for the server.

# Server

The brain of the entire operation. Uses a layered approach to separate graph modifications and optimizations with data transfer.

## Extra Functions

### serializeGraph

Responsible for serializing the graph. It takes the graph as an input and returns the serialized graph (string form).

### deserializeGraph

Responsible for deserializing the graph. It takes the serialized graph as an input and returns the graph object.

### handle_client_connection

Used for communicating with the nodes to share their own views of the total graph since no node initially knows the graph.
Will be explained in Layer 1 part of the algorithm

### handle_client_data_connection

Used for actually sharing the data over the graph as created by the handle_client_connection function. This function is also what communicates with the client.

### nodeThread

This is the thread that represents each node in the graph. It initializes 3 other threads: `threadListner`, `nodeDataListener` and `dataFwder`.
First is used to listen for new connections from other nodes for the graph building. Second is used to listen for data from the client and other nodes. Third is used to forward data to other nodes.
This thread is also what acts as the graphBroadcaster. Will be explained in the algorithm.

## Algorithm

### Layer 1

This layer consists of two threads `threadListener` and `nodeThread`.
The latter sends its current view of the graph to its neighbours so the neighbours can check and update themselves if required. It does this until it is sure that all nodes around it are updated. The former is more complicated. It is what handles the connection sent by `nodeThread` of other threads. It checks for differences within its own view of the graph and the view of the graph sent by the other node. If there is a difference, it updates its own view of the graph and lets `nodeThread` know that it has updated its view of the graph.

This process continues until all nodes have a full view of the graph

### Layer 2

This layer consists of two threads `nodeDataListener` and `dataFwder`. The former is responsible for listening for data from the client and other nodes. The latter is responsible for forwarding the data to other nodes. This is much simpler than the previous layer. The `nodeDataListener` thread simply listens for data from the client and other nodes. It then forwards the data to the `dataFwder` thread. The `dataFwder` thread then forwards the data to all the nodes that are connected to it.

# How would I handle server failure?

I am assuming a server failure means a node failure. We can add periodic `alive?` checks where each neighbour checks if their neighbours are alive. If not, they can remove the node from their graph. When that happens the nodes can be told to flush their current table and start the process again. For this the node that detects a dead node must broadcast it to its neighbours and spread it throughout the graph. This will cause the graph to be rebuilt and the data to be retransmitted if required.

#include "../../include/Client.hpp"

/* ----------------------- Constructor and Destructor ----------------------- */
Client::Client(ServerConfig& server, int fd, struct sockaddr_in& addr)
    : _client_socket(fd),           
      _request_buffer(),            
      _response_buffer(),           
      _request(),                   
      _server(server),             
      _client_addr(addr.sin_addr.s_addr), 
      _client_port(addr.sin_port)
{
}

Client::~Client() {}

/* --------------------------------- Getters -------------------------------- */
in_port_t	Client::getClientPort() const  { return _client_port; }
in_addr_t	Client::getClientAddr() const  { return _client_addr; }
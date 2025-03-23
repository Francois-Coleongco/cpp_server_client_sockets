#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sodium/crypto_kx.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

const size_t buffer_size = 4096;

std::vector<std::thread> threads;
std::vector<int> clients;
int numConnections = 0;

int crypt_gen(int client_sock, unsigned char *server_pk,
              unsigned char *server_sk, unsigned char *server_rx,
              unsigned char *server_tx) {

  /* Generate the server's key pair */
  crypto_kx_keypair(server_pk, server_sk);
  std::cerr << "this is server_pk" << std::endl;

  for (int i = 0; i < crypto_kx_PUBLICKEYBYTES; ++i) {
    printf("%c", server_pk[i]);
  }

  std::cout << std::endl;

  // send server_pk to client

  send(client_sock, server_pk, crypto_kx_PUBLICKEYBYTES, 0);

  // receive client_pk from client

  unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];

  recv(client_sock, client_pk, crypto_kx_PUBLICKEYBYTES, 0);

  std::cerr << "this is client_pk" << std::endl;
  for (int i = 0; i < crypto_kx_PUBLICKEYBYTES; ++i) {
    printf("%c", client_pk[i]);
  }

  std::cout << std::endl;

  /* Prerequisite after this point: the client's public key must be known by
   * the server */

  /* Compute two shared keys using the client's public key and the server's
     secret key. server_rx will be used by the server to receive data from
     the client, server_tx will be used by the server to send data to the
     client. */

  if (crypto_kx_server_session_keys(server_rx, server_tx, server_pk, server_sk,
                                    client_pk) != 0) {
    std::cerr << "BAILED" << std::endl;
    /* Suspicious client public key, bail out */
    return 1;
  }

  std::cerr << "DIDNT BAIL WE HAVE VALID KEYSSS YAYYY" << std::endl;
  return 0;
}

void forward_to_all(std::array<char, buffer_size> buffer, int sender) {

  for (int client_sock : clients) {

    if (client_sock == sender) {
      continue;
    }

    std::cout << "trying to forward buffer to client_sock: " << client_sock
              << std::endl;

    int bytes_sent = send(client_sock, buffer.data(), buffer_size, 0);

    if (bytes_sent < 0) {
      std::cerr << "couldn't forward to this client: " << client_sock
                << std::endl;
    }
  }
}

void kill_yourself_listen(char *c, int server_sock) {
  std::cout << "press q then ENTER to shutdown server" << std::endl;
  std::cin >> *c;
  std::cout << "connections spawned: " << numConnections << std::endl;
  close(server_sock);
  exit(1);
}

void handle_conn(int client_sock) {

  unsigned char server_pk[crypto_kx_PUBLICKEYBYTES],
      server_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char server_rx[crypto_kx_SESSIONKEYBYTES],
      server_tx[crypto_kx_SESSIONKEYBYTES];

  if (crypt_gen(client_sock, server_pk, server_sk, server_rx, server_tx)) {
    std::cerr << "couldn't gen keys :c" << std::endl;
  }

  std::cerr << "this is server_tx" << std::endl;

  for (int i = 0; i < crypto_kx_SESSIONKEYBYTES; ++i) {
    printf("%c", server_tx[i]);
  }

	std::cerr << std::endl;

  std::array<char, buffer_size> buffer{0};

  std::cout << "this is data\n";

  // make sure the last character the 4095th index is not overwritten
  // cuz this is the null pointer for you cstring
  std::cout << "reading firs time\n";

  int bytes_read = 1;

  while (bytes_read > 0) {
    std::cout << "we are going to read from socket id: " << client_sock
              << std::endl;
    bytes_read = recv(client_sock, &buffer, buffer_size, 0);
    std::cout << bytes_read << std::endl;
    std::cout.write(buffer.data(), buffer_size) << std::endl;
    forward_to_all(buffer, client_sock); // copy value cuz buffer could change
    std::fill(buffer.data(), buffer.data() + buffer_size, '\0');
  }
}

int main() {

  int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock == -1) {
    std::cout << "Failed to create socket descriptor. " << strerror(errno)
              << "\n";
    return 1;
  } else {
    std::cout << "success making server_sock\n";
  }
  sockaddr_in server_address;

  server_address.sin_family = AF_INET;

  server_address.sin_port = htons(8080);

  server_address.sin_addr.s_addr = INADDR_ANY; // 127.0.0.1
  std::cout << "starign bind\n";

  int opt = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    std::cout << "Failed to set SO_REUSEADDR. " << strerror(errno) << std::endl;
    close(server_sock);
    return 1;
  }

  int bind_stat = bind(server_sock, (struct sockaddr *)&server_address,
                       sizeof(server_address));

  if (bind_stat == -1) {
    std::cout << "Failed to bind socket. " << strerror(errno) << std::endl;
    close(server_sock);
    return 1;
  }

  if (listen(server_sock, 5) < 0) {
    std::cout << "server could not listen" << std::endl;
    close(server_sock);
  }

  std::cout << "accepting\n";

  char c = '\0';

  std::cout << "kill_yourself_listen" << std::endl;

  // start a thread to listen for 'q'
  std::thread(kill_yourself_listen, &c, server_sock).detach();

  while (true) {

    int client_sock = accept(server_sock, nullptr, nullptr);

    std::cout << "made another socket: " << client_sock << "\n";

    if (client_sock < 0) {
      std::cerr << "Failed to accept connection" << std::endl;
      continue;
    }

    threads.push_back(std::thread(handle_conn, client_sock));
    clients.push_back(client_sock);
    ++numConnections;

    threads.back().detach();
  }
}

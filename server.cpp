#include <array>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sys/socket.h>
#include <unistd.h>

const size_t buffer_size = 4096;

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

  listen(server_sock, 5);

  std::cout << "accepting\n";
  int client_sock = accept(server_sock, nullptr, nullptr);

  std::array<char, buffer_size> buffer{0};

  std::cout << "this is data\n";

  // make sure the last character the 4095th index is not overwritten
  // cuz this is the null pointer for you cstring
  std::cout << "reading firs time\n";

  int bytes_read = 1;

  while (bytes_read > 0) {
    bytes_read = recv(client_sock, &buffer, buffer_size, 0);
    std::cout << bytes_read << std::endl;
    std::cout.write(buffer.data(), buffer_size) << std::endl;
  }

  close(server_sock);
}

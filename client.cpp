#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const size_t buffer_size = 4096;

void read_msg(int client_sock) {

  std::cout << "called me maybe?" << client_sock << std::endl;

  std::array<char, buffer_size> buffer{0};

  while (true) {
    int bytes_read = recv(client_sock, &buffer, buffer_size, 0);

    std::cout << "ECHOOO" << std::endl;

    std::cout.write(buffer.data(), buffer_size) << std::endl;
  }
  std::fill(buffer.data(), buffer.data() + buffer_size, '\0');
}

int main() {

  const size_t buffer_size = 4096;

  int client_sock = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(8080);
  server_address.sin_addr.s_addr = INADDR_ANY;

  int conn_stat = connect(client_sock, (struct sockaddr *)&server_address,
                          sizeof(server_address));

  std::array<char, buffer_size> buffer{0};

  std::array<char, buffer_size> msg_box{0};

  char c = getchar();

  size_t msg_char_idx = 1;

  std::cout << "starting reader" << std::endl;
  std::thread reader(read_msg, client_sock);
  reader.detach();

  while (!feof(stdin)) {

    if (c == '\n' || msg_char_idx == buffer_size - 1) {
      // send the message then clear buffer THEN reset msg_char_idx
      int bytes_sent = send(client_sock, &msg_box, sizeof(msg_box), 0);

      if (bytes_sent < 0) {
        std::cout << "error sending client hello to server" << std::endl;
      }

      std::fill(msg_box.data(), msg_box.data() + buffer_size, '\0');
    }

    msg_box[msg_char_idx] = c;

    ++msg_char_idx;

    c = getchar();
  }

  close(client_sock);
  int bytes_read = recv(client_sock, &buffer, sizeof(buffer), 0);

  std::cout << "EXIT";
}

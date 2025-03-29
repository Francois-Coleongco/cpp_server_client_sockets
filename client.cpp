#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_kx.h>
#include <sodium/randombytes.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const size_t buffer_size = 4096;

int crypt_gen(int client_sock, unsigned char *client_pk,
              unsigned char *client_sk, unsigned char *client_rx,
              unsigned char *client_tx) {

  /* Generate the client's key pair */
  crypto_kx_keypair(client_pk, client_sk);

  std::cerr << "this is client_pk" << std::endl;

  for (int i = 0; i < crypto_kx_PUBLICKEYBYTES; ++i) {
    printf("%c", client_pk[i]);
  }

  std::cout << std::endl;

  send(client_sock, client_pk, crypto_kx_PUBLICKEYBYTES, 0);

  /* Prerequisite after this point: the server's public key must be known by the
   * client */

  /* Compute two shared keys using the server's public key and the client's
     secret key. client_rx will be used by the client to receive data from the
     server, client_tx will be used by the client to send data to the server. */

  unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];

  int crypto_bytes_read =
      recv(client_sock, server_pk, crypto_kx_PUBLICKEYBYTES, 0);

  std::cout << "ON THE CLIENT cryptobytesread: " << crypto_bytes_read
            << std::endl;

  std::cerr << "this is server_pk" << std::endl;

  for (int i = 0; i < crypto_kx_PUBLICKEYBYTES; ++i) {
    printf("%c", server_pk[i]);
  }

  std::cout << std::endl;

  if (crypto_kx_client_session_keys(client_rx, client_tx, client_pk, client_sk,
                                    server_pk) != 0) {
    std::cerr << "BAILED SUS KEYS" << std::endl;
    return 1;
    /* Suspicious server public key, bail out */
  }

  std::cerr << "DIDNT BAIL WE HAVE VALID KEYSSS YAYAYYYYY" << std::endl;
  return 0;
}

int encrypt_buffer(unsigned char *client_tx, unsigned char *msg_box,
                   int message_len, unsigned char *ciphertext,
                   unsigned long long *ciphertext_len, int client_sock) {

  unsigned char nonce[crypto_aead_chacha20poly1305_NPUBBYTES];

  randombytes_buf(nonce, sizeof nonce);

  send(client_sock, nonce, crypto_aead_chacha20poly1305_NPUBBYTES, 0);

  std::cerr << "sent the nonce" << std::endl;

  crypto_aead_chacha20poly1305_encrypt(ciphertext, ciphertext_len, msg_box,
                                       message_len, NULL, 0, NULL, nonce,
                                       client_tx);

  std::cerr << "this is the ciphertext_len" << ciphertext_len << std::endl;

  std::cerr << "okay so i sent with client_tx which is " << client_tx
            << "and nonce was " << nonce << std::endl;

  return 0;
}

int send_credentials(int client_sock, unsigned char *client_tx) {

  std::string username;
  std::string password;

  std::cout << "enter username:" << std::endl;
  std::cin >> username;
  std::cout << "enter password:" << std::endl;
  std::cin >> password;

  // encrypt the username and password and send it over to the server and wait
  // for a resposne

  unsigned char username_ciphertext[username.length() + 1 +
                                    crypto_aead_chacha20poly1305_ABYTES];
  unsigned long long username_ciphertext_len;

  unsigned char password_ciphertext[password.length() + 1 +
                                    crypto_aead_chacha20poly1305_ABYTES];

  unsigned long long password_ciphertext_len;

  if (encrypt_buffer(client_tx, (unsigned char *)password.data(),
                     password.length() + 1, password_ciphertext,
                     &password_ciphertext_len, client_sock)) {
    std::cerr << "couldn't encrypt error in encrypt_buffer" << std::endl;
  }

  // send(client_sock, username_ciphertext, username_ciphertext_len,
  //      0); // username
  // std::cout << "finished sending username" << std::endl;
	std::cout << "this was my size: " << password_ciphertext_len << std::endl;
  send(client_sock, password_ciphertext, password_ciphertext_len,
       0); // username
  std::cout << "finished sending password" << std::endl;

  return 0;
}

void read_msg(int client_sock) {

  std::cout << "called me maybe?" << client_sock << std::endl;

  std::array<char, buffer_size> buffer{0};

  while (true) {
    int bytes_read = recv(client_sock, &buffer, buffer_size, 0);

    // problem: cant find out which client it is based onthe client_sock for
    // some reason
    // std::cout << "ECHOOO in " << client_sock << std::endl;

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

  unsigned char client_pk[crypto_kx_PUBLICKEYBYTES],
      client_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char client_rx[crypto_kx_SESSIONKEYBYTES],
      client_tx[crypto_kx_SESSIONKEYBYTES];

  if (crypt_gen(client_sock, client_pk, client_sk, client_rx, client_tx)) {
    std::cerr << "error generating keys :(" << std::endl;
    return 1;
  }

  std::cerr << "this is client_tx" << std::endl;

  for (int i = 0; i < crypto_kx_SESSIONKEYBYTES; ++i) {
    printf("%c", client_tx[i]);
  }

  std::cerr << std::endl;

  if (send_credentials(client_sock, client_tx)) {
    std::cerr << "couldn't verify credentials" << std::endl;
  }

  std::array<char, buffer_size> buffer{0};

  std::array<char, buffer_size> msg_box{0};
  std::array<char, buffer_size> encrypted_msg_box{0};

  size_t msg_char_idx = 0;

  std::cout << "starting reader" << std::endl;
  std::thread reader(read_msg, client_sock);
  reader.detach();

  char c;

  while (std::cin >> std::noskipws >> c) {

    if (c == '\n' || msg_char_idx == buffer_size - 1) {
      std::cout << "attempted print" << std::endl;
      // send the message then clear buffer THEN reset msg_char_idx
      // should encrypt here the buffer msg_box before sending
      // i need a function that takes the client_tx and msg_box and returns the
      // ciphertext

      // must reserve part of the buffer for size of message
      // note we need to send the size becuase the actual mesasge might contain
      // the zero byte. therefore we can't just parse till the null byte for
      // fear of losing data

      int bytes_sent = send(client_sock, &msg_box, sizeof(msg_box), 0);
      msg_char_idx = 0; // set it back to zero for the next message

      if (bytes_sent < 0) {
        std::cout << "error sending client hello to server" << std::endl;
      }

      std::fill(msg_box.data(), msg_box.data() + buffer_size, '\0');
    }

    msg_box[msg_char_idx] = c;

    std::cout << msg_box[msg_char_idx] << std::endl;

    ++msg_char_idx;
  }
  close(client_sock);

  std::cout << "EXIT";
}

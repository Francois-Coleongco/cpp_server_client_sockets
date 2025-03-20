#include <functional>
#include <iostream>
#include <sqlite3.h>
#include <string>


int create_user(sqlite3 *DB, std::string *username, std::string *password) {

	// query for the username

	sqlite3_exec(DB)

}

int login(std::string *username, std::string *password) {

  int status = 0; // return 1 if success, 0 if not;
	//

  // match username
  //
  // match password

  // at the end, zero out the username and password vars;
}

int main() {

  sqlite3 *DB;

  int exit = 0;

  exit = sqlite3_open("term_chat_users.db", &DB);

  std::cout << exit << std::endl;

  std::hash<std::string> hasher;

  std::string username;
  std::string password;

  std::cin >> username;
  std::cin >> password;

  size_t hash = hasher(password);

  std::cout << "hash is: " << hash << std::endl;

  create_user(&username, &password);
}

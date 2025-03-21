#include <functional>
#include <iostream>
#include <sqlite3.h>
#include <string>

int create_user(sqlite3 *DB, std::string *username, std::string *password) {

  sqlite3_stmt *stmt;

  const char *check_exists_query =
      "SELECT COUNT(*) FROM users WHERE username = ?;";

	if (sqlite3_prepare_v2(DB, check_exists_query, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "unable to prepare check_exists_query" << sqlite3_errmsg(DB) << std::endl;
	}

	sqlite3_bind_text(stmt, 1, username->c_str(), -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		int count = sqlite3_column_int(stmt, 0);
		if (count > 0) {
			std::cerr << "USER EXISTS" << std::endl;
			return 1;
		}
	}


	sqlite3_finalize(stmt);

	return 0;

}


int main() {

  sqlite3 *DB;

  int exit = 0;

  exit = sqlite3_open("term_chat_users.db", &DB);

  if (exit != 0) {
    std::cerr << "couldn't open database" << std::endl;
    return 1;
  }

  std::hash<std::string> hasher;

  std::string username;
  std::string password;

	std::cout << "enter username:" << std::endl;
  std::cin >> username;
	std::cout << "enter password:" << std::endl;
  std::cin >> password;

  size_t hash = hasher(password);

  std::cout << "hash is: " << hash << std::endl;

	if (create_user(DB, &username, &password) != 0) {
		std::cerr << "user exists" << std::endl;
	}

}

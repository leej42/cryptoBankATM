#include <vector>

class userInfo;
class userDB;

std::string deposit(const std::string user, const std::string amount, userDB* users);
std::string balance(const std::string* user, userDB* users);

void login(const std::string* user, int pin);
void balance(const std::string* user);
void amount(const std::string* user, int amount);
void transfer(const std::string* user, int amount, const std::string* otherUser);
void logout(std::string user);


class userInfo {
  private:
    bool loggedIn;
    std::string name;
    std::string pin;
    int balance;
  public:
    userInfo(std::string n, std::string p, int b) {
      name = n;
      pin = p;
      balance = b;
      loggedIn = false;
    }
    userInfo() {}
    std::string getName() {return name;}
    bool hasName(std::string name) {
      if (this->name.compare(name) == 0) {
        return true;
      }
      return false;
    }
    std::string getBalance() {
      std::stringstream ss;
      ss << balance;
      return ss.str();
    }
    bool isLoggedIn() {
      return loggedIn;
    }
    bool login(std::string pin) {
      if (this->pin.compare(pin) == 0) {
        if (loggedIn) {
          // user cannot login if already logged in
          return false;
        }
        loggedIn = true;
        return true;
      }
      return false;
    }
    void logout() {
      loggedIn = false;
    }
    std::string withdraw(std::string amo) {
      int amount = atoi(amo.c_str());
      balance -= amount;
      std::stringstream ss;
      ss << amount;
      return ss.str();
    }
    void deposit(std::string amo) {
      int amount = atoi(amo.c_str());
      balance += amount;
    }
};

class userDB {
  private:
    std::vector<userInfo> users;
  public:
    userDB() {}
    void addUser(userInfo user) {
      users.push_back(user);
    }
    userInfo* findUser(std::string name) {
      for (int i = 0; i < users.size(); i++) {
        if (users[i].hasName(name)) {
          return &users[i];
        }
      }
      return NULL;
    }
    bool loginUser(std::string name, std::string pin) {
      userInfo *user;
      user = this->findUser(name);
      return user->login(pin);
    }
    void transfer(std::string transferer, std::string transferee, std::string amount) {
      userInfo* from = this->findUser(transferer);
      userInfo* to = this->findUser(transferee);
      from->withdraw(amount);
      to->deposit(amount);
      // deposit(transferee, amount, this);
    }
    bool userExists(std::string name) {
      if (this->findUser(name)) {
        return true;
      }
      return false;
    }
};
void init_bank(userDB* users) {
  userInfo a = userInfo("alice", "abc123", 50);
  userInfo b = userInfo("bob", "def456", 100);
  userInfo e = userInfo("eve", "ghi789", 0);
  users->addUser(a);
  users->addUser(b);
  users->addUser(e);
}


std::string deposit(const std::string user, const std::string amount, userDB* users) {
  userInfo *userVar;
  userVar = users->findUser(user);
  userVar->deposit(amount);
  return userVar->getBalance();
  // std::cout << "dtest" << std::endl;

}

std::string balance(const std::string user, userDB* users) {
  userInfo *userVar;
  userVar = users->findUser(user);
  return userVar->getBalance();
}

void login(const std::string* user, int pin) {

}

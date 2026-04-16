#include "Client.hpp"

// Default constructor
Client::Client(void) { return; }

Client::Client(std::string name, std::string email, std::string addr,
               std::string tel, std::string siret)
    : _name(name), _email(email), _addr(addr), _tel(tel), _siret(siret) {
  return;
}
// Copy constructor
Client::Client(const Client &other) { (void)other; }

// Assignment operator overload
Client &Client::operator=(const Client &other) {
  (void)other;
  return (*this);
}

// Destructor
Client::~Client(void) {}

void Client::set_name(const std::string &n) { _name = n; }

void Client::set_email(const std::string &e) { _email = e; }

void Client::set_addr(const std::string &a) { _addr = a; }

void Client::set_tel(const std::string &t) { _tel = t; }

void Client::set_siret(const std::string &s) { _siret = s; }

std::string Client::get_name() { return _name; }
std::string Client::get_email() { return _email; }
std::string Client::get_addr() { return _addr; }
std::string Client::get_tel() { return _tel; }
std::string Client::get_siret() { return _siret; }

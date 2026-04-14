#ifndef CLIENT_HPP
# define CLIENT_HPP
# include <iostream>

class Client
{
    public:
        ~Client();
        Client(void);
		Client(std::string name, std::string email, std::string addr = "", std::string tel = "", std::string siret = "");

		void set_name(const std::string&);
		void set_email(const std::string&);
		void set_addr(const std::string&);
		void set_tel(const std::string&);
		void set_siret(const std::string&);

		std::string get_name();
		std::string get_email();
		std::string get_addr();
		std::string get_tel();
		std::string get_siret();
	private:
		std::string _name;
		std::string _email;
		std::string _addr;
		std::string _tel;
		std::string _siret;
        Client(const Client& other);
        Client &operator=(const Client &other);
};

#endif


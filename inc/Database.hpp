#ifndef DATABASE_HPP
# define DATABASE_HPP
# include <iostream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include "Facture.hpp"
class Facture;
class Risography;
class Client;
class Facturier;

class Database {
	public:
		Database(const std::string& path, Facturier* f);
		~Database();

		enum PrintMode {
			PREVIEW,
			COMPLETE
		};


		bool delete_document(sqlite3_int64 document_id);
		std::vector<CustomLine> get_customs(sqlite3_int64 doc_id);
		bool insert_custom(const CustomLine& line, sqlite3_int64);
		bool numero_exists(const std::string& numero);
		std::string make_unique_numero(const std::string& base_numero);
		bool update_document(sqlite3_int64 document_id, Facture::TYPE type,Facture::STATUS status, const std::string& notes = "");
		// bool update_document_number(sqlite3_int64 document_id, const std::string& number); 
		void browse_documents();
		sqlite3_int64 insert_document(Facture* f, sqlite3_int64 client_id);
		void insert_risography(Risography *riso, sqlite3_int64 document_id);
		sqlite3_int64 insert_client(Client* f);
		void save_facture(Facture* f);
		Client *get_client(sqlite3_int64 client_id);
		Facture* get_document(sqlite3_int64 doc_id);
		std::vector<Risography*> get_risography_of(sqlite3_int64 document_id);
		// void print_all_documents(PrintMode mode = COMPLETE);
		void print_all_documents(PrintMode mode = PREVIEW, std::optional<Facture::TYPE> type_filter = std::nullopt);
		bool print_document(sqlite3_int64 document_id, PrintMode mode = COMPLETE);
		void print_client(sqlite3_int64 client_id, PrintMode mode = COMPLETE);
		void print_risography(sqlite3_int64 document_id, PrintMode mode = COMPLETE);
	private:
		sqlite3* _db;
		Facturier* _facturier;

		// Exécute une requête SQL sans résultat attendu
		void exec(const std::string& sql);

		void initialize();

		void createTableClients();
		void createTableDocuments();
		void createTableLignesCustom();
		void createTableLignesRiso();
		void createTableLignesSerigraphie();
		void createTableMetadonnees();
};

#endif


#ifndef DATABASE_HPP
# define DATABASE_HPP
# include <iostream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
class Facture;
class Database {
	public:
		Database(const std::string& path);
		~Database();

		enum PrintMode {
			PREVIEW,
			COMPLETE
		};
		sqlite3_int64 insert_document(Facture* f, sqlite3_int64 client_id);
		void insert_risography(Facture* f, sqlite3_int64 document_id);
		sqlite3_int64 insert_client(Facture* f);
		void save_facture(Facture* f);

		void print_all_documents(PrintMode mode = COMPLETE);
		void print_document(sqlite3_int64 document_id, PrintMode mode = COMPLETE);
		void print_client(sqlite3_int64 client_id, PrintMode mode = COMPLETE);
		void print_risography(sqlite3_int64 document_id, PrintMode mode = COMPLETE);
	private:
		sqlite3* db_;

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
// class Database
// {
//     public:
//         ~Database();
// 	private:
//         Database(void);
//         Database(const Database& other);
//         Database &operator=(const Database &other);
// };

#endif


#include "Database.hpp"
#include "Facture.hpp"
#include <iomanip>
// Default constructor
// Database::Database(void) {return ;}
Database::Database(const std::string& path): db_(nullptr) {
        int rc = sqlite3_open(path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Impossible d'ouvrir la base : "
                                     + std::string(sqlite3_errmsg(db_)));
        }
        // Active les clés étrangères à chaque connexion
        exec("PRAGMA foreign_keys = ON;");
        initialize();
    }

Database::~Database() {
        sqlite3_close(db_);
    }

// Exécute une requête SQL sans résultat attendu
void Database::exec(const std::string& sql) {
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK) {
		std::string err = errMsg;
		sqlite3_free(errMsg);
		throw std::runtime_error("Erreur SQL : " + err);
	}
}

void Database::initialize() {
	createTableClients();
	createTableDocuments();
	createTableLignesCustom();
	createTableLignesRiso();
	createTableLignesSerigraphie();
	createTableMetadonnees();
}

void Database::createTableClients() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS clients (
			id        INTEGER PRIMARY KEY AUTOINCREMENT,
			nom       TEXT    NOT NULL,
			adresse   TEXT,
			email     TEXT,
			telephone TEXT,
			siret     TEXT
		);
	)");
}

void Database::createTableDocuments() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS documents (
			id             INTEGER PRIMARY KEY AUTOINCREMENT,
			client_id      INTEGER NOT NULL,
			type           TEXT    NOT NULL CHECK(type IN ('devis','facture', 'ticket')),
			numero         TEXT    NOT NULL UNIQUE,
			date_creation  TEXT    NOT NULL DEFAULT (date('now')),
			date_echeance  TEXT,
			statut         TEXT    NOT NULL DEFAULT 'brouillon'
								   CHECK(statut IN ('draft','sent','approved','converted')),
			notes          TEXT,
			FOREIGN KEY (client_id) REFERENCES clients(id)
		);
	)");
}

void Database::createTableLignesCustom() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS lignes_custom (
			id            INTEGER PRIMARY KEY AUTOINCREMENT,
			document_id   INTEGER NOT NULL,
			description   TEXT    NOT NULL,
			prix_unitaire REAL    NOT NULL,
			quantite      REAL    NOT NULL DEFAULT 1.0,
			FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
		);
	)");
}

void Database::createTableLignesRiso() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS lignes_riso (
			id            	INTEGER PRIMARY KEY AUTOINCREMENT,
			document_id  	INTEGER NOT NULL,
			description		TEXT,
			format        	REAL,
			couleurs_recto   INTEGER NOT NULL DEFAULT 1,
			couleurs_verso   INTEGER,
			papier        	TEXT,
			grammage        TEXT,
			nb_copies     	INTEGER NOT NULL DEFAULT 1,
			nb_agrafes     	INTEGER,
			nb_coupes     	INTEGER,
			nb_plis     	INTEGER,
			nb_graphisme	REAL,
			nb_imposition	REAL,
			port 			BOOL,
			remise			REAL,
			prix_papier		REAL,
			prix_encre		REAL,
			prix_matrice	REAL,
			prix_faconnage	REAL,
			prix_labeur		REAL,
			prix_graphisme	REAL,
			prix_imposition	REAL,
			FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
		);
	)");
}

void Database::createTableLignesSerigraphie() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS lignes_serigraphie (
			id             INTEGER PRIMARY KEY AUTOINCREMENT,
			document_id    INTEGER NOT NULL,
			format         TEXT,
			nb_copies      INTEGER NOT NULL DEFAULT 1,
			nb_couleurs    INTEGER NOT NULL DEFAULT 1,
			support        TEXT    CHECK(support IN ('textile','papier','autre')),
			couleurs_detail TEXT,
			taille         TEXT,
			prix_unitaire  REAL    NOT NULL,
			quantite       REAL    NOT NULL DEFAULT 1.0,
			FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
		);
	)");
}

void Database::createTableMetadonnees() {
	exec(R"(
		CREATE TABLE IF NOT EXISTS metadonnees (
			id          INTEGER PRIMARY KEY AUTOINCREMENT,
			entity_type TEXT NOT NULL,
			entity_id   INTEGER NOT NULL,
			cle         TEXT NOT NULL,
			valeur      TEXT
		);
	)");
}

sqlite3_int64 Database::insert_client(Facture* facture) {
	sqlite3_stmt* stmt;

	if (!facture) {
		std::cerr << "No facture to save\n";
		return 0;
	}

	const char* sql = R"(
		INSERT INTO clients (nom, adresse, email, telephone, siret)
		VALUES (?, ?, ?, ?, ?);
	)";
	sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

	// 2. Bind les valeurs (index commence à 1)
	sqlite3_bind_text(stmt, 1, facture->client_name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, facture->client_addr.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, facture->client_email.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, facture->client_tel.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_null (stmt, 5); // siret vide

	// Execute
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "Insert client failed\n";
	} else {
		std::cout << "Insert client success\n";
	}

	sqlite3_finalize(stmt);
	sqlite3_int64 new_id = sqlite3_last_insert_rowid(db_);

	return new_id;
}
void Database::insert_risography(Facture* f, sqlite3_int64 document_id) {
    if (!f) return;

    if (document_id == 0) {
        std::cerr << "insert_risography: document not found for numero=" << f->number << '\n';
        return;
    }

    // Insert each Risography in the vector
    const char* sql = R"(
        INSERT INTO lignes_riso (
			document_id  	,
			description		,
			format        	,
			couleurs_recto  ,
			couleurs_verso  ,
			papier        	,
			grammage        ,
			nb_copies     	,
			nb_agrafes     	,
			nb_coupes     	,
			nb_plis     	,
			nb_graphisme	,
			nb_imposition	,
			port 			,
			remise			,
			prix_papier		,
			prix_encre		,
			prix_matrice	,
			prix_faconnage	,
			prix_labeur		,
			prix_graphisme	,
			prix_imposition	
        ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
    )";

    for (Risography* riso : f->risoprints) {
        if (!riso) continue;

        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

        sqlite3_bind_int64(stmt,  1, document_id);
        sqlite3_bind_text  (stmt, 2, riso->description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, riso->format);
        sqlite3_bind_int   (stmt, 4, riso->layersA);          // couleur_recto
        sqlite3_bind_int   (stmt, 5, riso->layersB);          // couleur_verso
        sqlite3_bind_text  (stmt, 6, riso->paper.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int   (stmt, 7, riso->sheet_weight);     // nb_copies
        sqlite3_bind_int   (stmt, 8, riso->copy);             // nb_copies
        sqlite3_bind_int   (stmt, 9, riso->staples_per_unit);   // nb_plis
        sqlite3_bind_int   (stmt, 10, riso->cut_per_unit);   // nb_plis
        sqlite3_bind_int   (stmt, 11, riso->fold_per_unit);   // nb_plis
        sqlite3_bind_double(stmt, 12, riso->graphic_amount);    // nb_graphisme
        sqlite3_bind_double(stmt, 13, riso->imposition_amount); // nb_imposition
        sqlite3_bind_double(stmt, 14, riso->shipping_fees);  // nb_labeur
        sqlite3_bind_double(stmt, 15, riso->discount_percentage);  // nb_labeur
        sqlite3_bind_double(stmt, 16, riso->sheet_price);   // prix_papier
        sqlite3_bind_double(stmt, 17, riso->ink_price);  // prix_matrice
        sqlite3_bind_double(stmt, 18, riso->master_price);     // prix_encre
        sqlite3_bind_double(stmt, 19, riso->shaping_price); // prix_faconnage
        sqlite3_bind_double(stmt, 20, riso->labor_price);   // prix_labeur
        sqlite3_bind_double(stmt, 21, riso->graphic_price); // prix_graphisme
        sqlite3_bind_double(stmt, 22, riso->imposition_price); // prix_imposition

        if (sqlite3_step(stmt) != SQLITE_DONE)
            std::cerr << "insert_risography failed: " << sqlite3_errmsg(db_) << '\n';

        sqlite3_finalize(stmt);
    }
}

void Database::save_facture(Facture* f) {
    if (!f) return;
    sqlite3_int64 client_id = insert_client(f);
    sqlite3_int64 doc_id = insert_document(f, client_id);
    insert_risography(f, doc_id);
    // insert_customs(f, doc_id);
}

sqlite3_int64 Database::insert_document(Facture* facture, sqlite3_int64 client_id) {
		
	if (!facture) return 0;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = R"(
		INSERT INTO documents (client_id, type, numero, date_creation, date_echeance, statut, notes)
		VALUES (?, ?, ?, ?, ?, ?, ?);
	)";
	std::string type = facture->type == Facture::TYPE::FACTURE ? "facture" : "devis";
	sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

	sqlite3_bind_int64(stmt, 1, client_id);
	sqlite3_bind_text(stmt, 2, type.c_str(), 							-1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, facture->number.c_str(), 				-1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, facture->get_formated_date().c_str(),	-1, SQLITE_TRANSIENT);
	sqlite3_bind_null(stmt, 5);
	sqlite3_bind_text(stmt, 6, "draft", 								-1, SQLITE_TRANSIENT);
	sqlite3_bind_null(stmt, 7);
	// Execute
	if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "insert document failed: " << sqlite3_errmsg(db_) << '\n';
        sqlite3_finalize(stmt);
        return 0;
    } else {
		std::cout << "insert document success\n";
	}

	sqlite3_finalize(stmt);
	sqlite3_int64 new_id = sqlite3_last_insert_rowid(db_);
	return new_id;
}

void Database::print_client(sqlite3_int64 client_id, PrintMode mode) {
    const char* sql = "SELECT id, nom, adresse, email, telephone, siret FROM clients WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, client_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
		if (mode == COMPLETE) {
        std::cout << "  CLIENT\n";
        std::cout << "    id        : " << sqlite3_column_int64(stmt, 0) << '\n';
        std::cout << "    nom       : " << (sqlite3_column_text(stmt, 1) ? (const char*)sqlite3_column_text(stmt, 1) : "") << '\n';
        std::cout << "    adresse   : " << (sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "") << '\n';
        std::cout << "    email     : " << (sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "") << '\n';
        std::cout << "    telephone : " << (sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "") << '\n';
        std::cout << "    siret     : " << (sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "") << '\n';
		} else {
        	std::cout << "à    : " << (sqlite3_column_text(stmt, 1) ? (const char*)sqlite3_column_text(stmt, 1) : "") << '\n';
		}
    }
    sqlite3_finalize(stmt);
}

void Database::print_risography(sqlite3_int64 document_id, PrintMode mode) {
    const char* sql = R"(
        SELECT
            id,
            description,
            format,
            couleurs_recto,
            couleurs_verso,
            papier,
            grammage,
            nb_copies,
            nb_agrafes,
            nb_coupes,
            nb_plis,
            nb_graphisme,
            nb_imposition,
            port,
            remise,
            prix_papier,
            prix_encre,
            prix_matrice,
            prix_faconnage,
            prix_labeur,
            prix_graphisme,
            prix_imposition
        FROM lignes_riso WHERE document_id = ?;
    )";
    //   0    1            2       3               4               5       6
    //   7         8          9         10       11            12
    //   13    14      15          16          17           18
    //   19            20          21              22

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "print_risography prepare failed: " << sqlite3_errmsg(db_) << '\n';
        return;
    }
    sqlite3_bind_int64(stmt, 1, document_id);

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ++count;

        auto txt = [&](int col) -> std::string {
            auto* t = sqlite3_column_text(stmt, col);
            return t ? reinterpret_cast<const char*>(t) : "";
        };

        std::string desc      = txt(1);
        double      format    = sqlite3_column_double(stmt, 2);
        int         layersA   = sqlite3_column_int   (stmt, 3);
        int         layersB   = sqlite3_column_int   (stmt, 4);
        std::string paper     = txt(5);
        int         grammage  = sqlite3_column_int   (stmt, 6);
        int         copy      = sqlite3_column_int   (stmt, 7);
        int         staples   = sqlite3_column_int   (stmt, 8);
        int         cut       = sqlite3_column_int   (stmt, 9);
        int         fold      = sqlite3_column_int   (stmt, 10);
        double      graphism  = sqlite3_column_double(stmt, 11);
        double      impos     = sqlite3_column_double(stmt, 12);
        int         shipping  = sqlite3_column_int   (stmt, 13);
        double      discount  = sqlite3_column_double(stmt, 14);
        double      sheetP    = sqlite3_column_double(stmt, 15);
        double      inkP      = sqlite3_column_double(stmt, 16);
        double      masterP   = sqlite3_column_double(stmt, 17);
        double      shapingP  = sqlite3_column_double(stmt, 18);
        double      laborP    = sqlite3_column_double(stmt, 19);
        double      graphicP  = sqlite3_column_double(stmt, 20);
        double      imposP    = sqlite3_column_double(stmt, 21);

        Risography r(desc, paper, format, copy, layersA, layersB,
                     staples, cut, fold, graphism, graphism, impos,
                     discount, shipping, inkP, sheetP, masterP,
                     graphicP, shapingP, imposP, laborP, grammage);

        r.calc_quantity();
		r.rest_to_pay();
		if (mode == COMPLETE) {
			std::cout << r.generate_stream();
		} else {
			std::cout << "RISO : " << r.description << "\n";
			std::cout << "total: " << r.rest_to_pay() << "\n";
		}
    }

    if (count == 0)
        std::cout << "  RISO : (aucune ligne)\n";

    sqlite3_finalize(stmt);
}

void Database::print_document(sqlite3_int64 document_id, PrintMode mode) {
    const char* sql = R"(
        SELECT id, client_id, type, numero, date_creation, date_echeance, statut, notes
        FROM documents WHERE id = ?;
    )";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "prepare failed: " << sqlite3_errmsg(db_) << '\n';
        return;
    }

    sqlite3_bind_int64(stmt, 1, document_id);

    sqlite3_int64 client_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {              // ← step FIRST
		if (mode == COMPLETE) {
			client_id = sqlite3_column_int64(stmt, 1);       // ← THEN read columns
			std::cout << "----------------------------------------------------------\n";
			std::cout << "  client id     : " << client_id << '\n';
			std::cout << "  type          : " << (sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "") << '\n';
			std::cout << "  numero        : " << (sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "") << '\n';
			std::cout << "  date creation : " << (sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "") << '\n';
			std::cout << "  date echeance : " << (sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "—") << '\n';
			std::cout << "  statut        : " << (sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "") << '\n';
			std::cout << "  notes         : " << (sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "—") << '\n';
		} else {
			std::cout << fit(document_id, 2) << ":";
			std::cout << (sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "") << " n°";
			std::cout << (sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "") << " du ";
			std::cout << (sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "");
			std::cout << " -----------------\n";
		}
    } else {
        std::cout << "document id=" << document_id << " not found\n";
    }
    sqlite3_finalize(stmt);                              // ← finalize BEFORE nested calls

    print_client(client_id, mode);
    print_risography(document_id, mode);
}

void Database::print_all_documents(PrintMode mode) {
	std::cout << "Print all documents\n";
    const char* sql = "SELECT id FROM documents ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    // Collect all ids first, close the statement, THEN print
    std::vector<sqlite3_int64> ids;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        ids.push_back(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt); // ← closed before any nested query

    for (sqlite3_int64 id : ids)
        print_document(id, mode);
}

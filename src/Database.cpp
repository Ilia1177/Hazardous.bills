#include "Database.hpp"
#include "Facture.hpp"
#include "Font.hpp"
#include "PrintFact.hpp"
#include "tools.h"
#include <iomanip>
#include <unistd.h>
// Default constructor
// Database::Database(void) {return ;}
Database::Database(const std::string &path, Facturier *f)
    : _db(nullptr), _facturier(f) {
  int rc = sqlite3_open(path.c_str(), &_db);
  if (rc != SQLITE_OK) {
    throw std::runtime_error("Impossible d'ouvrir la base : " +
                             std::string(sqlite3_errmsg(_db)));
  }
  // Active les clés étrangères à chaque connexion
  exec("PRAGMA foreign_keys = ON;");
  initialize();
}

Database::~Database() { sqlite3_close(_db); }

// Exécute une requête SQL sans résultat attendu
void Database::exec(const std::string &sql) {
  char *errMsg = nullptr;
  int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
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
			email     TEXT,
			adresse   TEXT,
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
			statut         TEXT    NOT NULL DEFAULT 'draft'
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

sqlite3_int64 Database::insert_document(Facture *facture,
                                        sqlite3_int64 client_id) {
  if (!facture)
    return 0;

  // Resolve unique numero before inserting
  std::string numero = make_unique_numero(facture->number);
  if (numero != facture->number) {
    std::cout << "numero conflict, using " << numero << " instead of "
              << facture->number << '\n';
    facture->number = numero; // update the facture object too
  }

  const char *sql = R"(
        INSERT INTO documents (client_id, type, numero, date_creation, date_echeance, statut, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

  std::string type, status;
  switch (facture->type) {
  case Facture::TYPE::FACTURE:
    type = "facture";
    break;
  case Facture::TYPE::DEVIS:
    type = "devis";
    break;
  case Facture::TYPE::TICKET:
    type = "ticket";
    break;
  }

  switch (facture->status) {
  case Facture::STATUS::DRAFT:
    status = "draft";
  case Facture::STATUS::SENT:
    status = "sent";
  case Facture::STATUS::APPROVED:
    status = "approved";
  case Facture::STATUS::CONVERTED:
    status = "converted";
  }

  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  sqlite3_bind_int64(stmt, 1, client_id);
  sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, numero.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, facture->get_formated_date().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_null(stmt, 5);
  sqlite3_bind_text(stmt, 6, status.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_null(stmt, 7);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "insert_document failed: " << sqlite3_errmsg(_db) << '\n';
    sqlite3_finalize(stmt);
    return 0;
  }

  std::cout << "insert document success — numero=" << numero << '\n';
  sqlite3_finalize(stmt);
  return sqlite3_last_insert_rowid(_db);
}

bool Database::numero_exists(const std::string &numero) {
  const char *sql = "SELECT 1 FROM documents WHERE numero = ? LIMIT 1;";
  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, numero.c_str(), -1, SQLITE_TRANSIENT);
  bool exists = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return exists;
}

std::string Database::make_unique_numero(const std::string &base_numero) {
  if (!numero_exists(base_numero))
    return base_numero;

  // "260411-045D" → prefix="260411-", hex="045", suffix='D'
  std::string prefix = base_numero.substr(0, 7); // "260411-"
  std::string hex = base_numero.substr(7, 3);    // "045"
  char suffix = base_numero.back();              // 'D' — never changes

  int counter = std::stoi(hex, nullptr, 16); // 0x045 = 69

  for (int i = 0; i < 4096; ++i) {
    ++counter;
    std::ostringstream oss;
    oss << prefix << std::uppercase << std::hex << std::setw(3)
        << std::setfill('0') << counter
        << suffix; // ← always reattach the same suffix
    std::string candidate = oss.str();
    if (!numero_exists(candidate))
      return candidate;
  }

  throw std::runtime_error("make_unique_numero: no free numero found");
}

sqlite3_int64 Database::insert_client(Client *client) {
  if (!client) {
    throw std::logic_error("no client");
  }

  sqlite3_stmt *stmt;
  const char *sql = R"(
		INSERT INTO clients (nom, email, adresse, telephone, siret)
		VALUES (?, ?, ?, ?, ?);
	)";

  sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);

  // 2. Bind les valeurs (index commence à 1)
  sqlite3_bind_text(stmt, 1, client->get_name().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, client->get_email().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, client->get_addr().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, client->get_tel().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, client->get_siret().c_str(), -1, SQLITE_TRANSIENT);

  // Execute
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Insert client failed\n";
  } else {
    std::cout << "Insert client success\n";
  }

  sqlite3_finalize(stmt);

  return sqlite3_last_insert_rowid(_db);
}

void Database::insert_risography(Risography *riso, sqlite3_int64 document_id) {
  if (!riso)
    return;

  // Insert each Risography in the vector
  const char *sql = R"(
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

  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);

  sqlite3_bind_int64(stmt, 1, document_id);
  sqlite3_bind_text(stmt, 2, riso->description.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 3, riso->format);
  sqlite3_bind_int(stmt, 4, riso->layersA); // couleur_recto
  sqlite3_bind_int(stmt, 5, riso->layersB); // couleur_verso
  sqlite3_bind_text(stmt, 6, riso->paper.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 7, riso->sheet_weight);            // nb_copies
  sqlite3_bind_int(stmt, 8, riso->copy);                    // nb_copies
  sqlite3_bind_int(stmt, 9, riso->staples_per_unit);        // nb_plis
  sqlite3_bind_int(stmt, 10, riso->cut_per_unit);           // nb_plis
  sqlite3_bind_int(stmt, 11, riso->fold_per_unit);          // nb_plis
  sqlite3_bind_double(stmt, 12, riso->graphic_amount);      // nb_graphisme
  sqlite3_bind_double(stmt, 13, riso->imposition_amount);   // nb_imposition
  sqlite3_bind_double(stmt, 14, riso->shipping_fees);       // nb_labeur
  sqlite3_bind_double(stmt, 15, riso->discount_percentage); // nb_labeur
  sqlite3_bind_double(stmt, 16, riso->sheet_price);         // prix_papier
  sqlite3_bind_double(stmt, 17, riso->ink_price);           // prix_matrice
  sqlite3_bind_double(stmt, 18, riso->master_price);        // prix_encre
  sqlite3_bind_double(stmt, 19, riso->shaping_price);       // prix_faconnage
  sqlite3_bind_double(stmt, 20, riso->labor_price);         // prix_labeur
  sqlite3_bind_double(stmt, 21, riso->graphic_price);       // prix_graphisme
  sqlite3_bind_double(stmt, 22, riso->imposition_price);    // prix_imposition

  if (sqlite3_step(stmt) != SQLITE_DONE)
    std::cerr << "insert_risography failed: " << sqlite3_errmsg(_db) << '\n';

  sqlite3_finalize(stmt);
}

std::vector<CustomLine> Database::get_customs(sqlite3_int64 doc_id) {

  std::vector<CustomLine> result;

  const char *sql = R"(
        SELECT description, quantite, prix_unitaire
        FROM lignes_custom WHERE document_id = ?;
    )";
  //     0            1         2

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "get_customs prepare failed: " << sqlite3_errmsg(_db) << '\n';
    return result;
  }

  sqlite3_bind_int64(stmt, 1, doc_id);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    auto *t = sqlite3_column_text(stmt, 0);
    std::string desc = t ? reinterpret_cast<const char *>(t) : "";
    int quantite = sqlite3_column_int(stmt, 1);
    double prix_unitaire = sqlite3_column_double(stmt, 2);

    result.emplace_back(desc, std::make_pair(quantite, prix_unitaire));
  }

  sqlite3_finalize(stmt);
  return result;
}

bool Database::insert_custom(const CustomLine &line, sqlite3_int64 doc_id) {
  const char *sql = R"(
        INSERT INTO lignes_custom (document_id, description, prix_unitaire, quantite)
        VALUES (?, ?, ?, ?);
    )";

  const std::string &description = line.first;
  const int quantite = line.second.first;
  const double prix_unitaire = line.second.second;

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "insert_custom prepare failed: " << sqlite3_errmsg(_db)
              << '\n';
    return false;
  }

  sqlite3_bind_int64(stmt, 1, doc_id);
  sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 3, prix_unitaire);
  sqlite3_bind_int(stmt, 4, quantite);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok)
    std::cerr << "insert_custom failed: " << sqlite3_errmsg(_db) << '\n';

  sqlite3_finalize(stmt);
  return ok;
}

void Database::browse_documents() {

  int selection = 0;
  while (!g_interrupt) {
    clearScreen();
    print_all_documents(PREVIEW);
    user_value("Select a document", selection);
    Facture *f = get_document(selection);
    if (!f)
      continue;

    do {
      clearScreen();
      std::cout << f->generate_stream();
      int choice = dial_menu({"Exit", "Ajouter riso print", "Ajouter une ligne",
                              "Convertir en facture", "Imprimer en PNG",
                              "Envoyer par email", "Supprimer"});
      switch (choice) {
      case 1:
        insert_risography(f->add_riso_print(), selection);
        break;
      case 2:
        insert_custom(f->add_custom_line(), selection);
        break;
      case 3:
        if (f->type == Facture::TYPE::FACTURE) {
          clearInputLine();
          std::cout << "Le document est déja enregistré en tant que facture";
          break;
        }
        update_document(selection, Facture::TYPE::FACTURE,
                        Facture::STATUS::CONVERTED);
        break;
      case 4: {
        std::string folder =
            f->type == Facture::TYPE::FACTURE ? "factures/" : "devis/";
        folder = "documents/" + folder;
        make_png(f->generate_stream(), _facturier->get_font(),
                 folder + f->get_filename());
        break;
      }
      case 5: {
        std::string folder =
            f->type == Facture::TYPE::FACTURE ? "factures/" : "devis/";
        folder = "documents/" + folder;
        make_png(f->generate_stream(), _facturier->get_font(),
                 folder + f->get_filename());
        send_email_smtp(
            f->client->get_email(), "Devis " + f->number, EMAIL_BODY,
            folder + f->get_filename(), _facturier->get_config("from"),
            _facturier->get_config("url"), _facturier->get_config("user"),
            _facturier->get_config("password"));
        break;
      }
      case 6:
        clearScreen();
        f->generate_stream();
        if (userline("are you sure to delete ? (y/n)") == "y")
          delete_document(selection);
      case 0:
      default:
        delete f;
        return;
      }
      delete f;
      f = get_document(selection);
    } while (!g_interrupt);
  }
}

void Database::save_facture(Facture *f) {
  if (!f)
    return;
  sqlite3_int64 client_id = insert_client(f->client);
  sqlite3_int64 doc_id = insert_document(f, client_id);

  for (auto riso : f->riso_prints)
    insert_risography(riso, doc_id);
  for (auto line : f->customs)
    insert_custom(line, doc_id);
}

Client *Database::get_client(sqlite3_int64 client_id) {
  const char *sql = R"(
        SELECT id, nom, adresse, email, telephone, siret
        FROM clients WHERE id = ?;
    )";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "get_client prepare failed: " << sqlite3_errmsg(_db) << '\n';
    return nullptr;
  }
  sqlite3_bind_int64(stmt, 1, client_id);

  auto txt = [&](int col) -> std::string {
    auto *t = sqlite3_column_text(stmt, col);
    return t ? reinterpret_cast<const char *>(t) : "";
  };

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string name = txt(1);
    std::string addr = txt(2);
    std::string email = txt(3);
    std::string tel = txt(4);
    std::string siret = txt(5);
    sqlite3_finalize(stmt);

    if (name.empty())
      throw std::logic_error("No name for this client\n");
    // make sure this order matches your Client constructor exactly
    return new Client(name, email, addr, tel, siret);
  }

  std::cerr << "get_client: no client found with id=" << client_id << '\n';
  sqlite3_finalize(stmt);
  return nullptr;
}

Facture *Database::get_document(sqlite3_int64 document_id) {
  const char *sql = R"(
        SELECT id, client_id, type, numero, date_creation, date_echeance, statut, notes
        FROM documents WHERE id = ?;
    )";
  sqlite3_stmt *stmt = nullptr;

  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "prepare failed: " << sqlite3_errmsg(_db) << '\n';
    return nullptr;
  }
  sqlite3_bind_int64(stmt, 1, document_id);

  sqlite3_int64 client_id = 0;

  auto txt = [&](int col) -> std::string {
    auto *t = sqlite3_column_text(stmt, col);
    return t ? reinterpret_cast<const char *>(t) : "";
  };
  std::string type;
  std::string numero;
  std::string date;
  std::string status;
  if (sqlite3_step(stmt) == SQLITE_ROW) {      // ← step FIRST
    client_id = sqlite3_column_int64(stmt, 1); // ← THEN read columns
    type = txt(2);
    numero = txt(3);
    date = txt(4);
    status = txt(6);
  } else {
    std::cout << "document id=" << document_id << " not found\n";
    sqlite3_finalize(stmt);
    return nullptr;
  }
  sqlite3_finalize(stmt);

  Facture *f = new Facture(_facturier);

  f->number = numero;
  f->date = date;
  if (status == "draft") {
    f->status = Facture::STATUS::DRAFT;
  } else if (status == "sent") {
    f->status = Facture::STATUS::SENT;
  } else if (status == "approved") {
    f->status = Facture::STATUS::APPROVED;
  } else {
    f->status = Facture::STATUS::CONVERTED;
  }
  if (type == "facture")
    f->type = Facture::TYPE::FACTURE;
  else if (type == "devis")
    f->type = Facture::TYPE::DEVIS;
  else
    f->type = Facture::TYPE::TICKET;

  f->riso_prints = get_risography_of(document_id);
  f->client = get_client(client_id);
  if (!f->client)
    throw std::logic_error("no client found");

  f->customs = get_customs(document_id);
  return f;
}

std::vector<Risography *>
Database::get_risography_of(sqlite3_int64 document_id) {
  const char *sql = R"(
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

  std::vector<Risography *> risoPrints;
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "print_risography prepare failed: " << sqlite3_errmsg(_db)
              << '\n';
    return risoPrints;
  }
  sqlite3_bind_int64(stmt, 1, document_id);

  while (sqlite3_step(stmt) == SQLITE_ROW) {

    auto txt = [&](int col) -> std::string {
      auto *t = sqlite3_column_text(stmt, col);
      return t ? reinterpret_cast<const char *>(t) : "";
    };

    std::string desc = txt(1);
    double format = sqlite3_column_double(stmt, 2);
    int layersA = sqlite3_column_int(stmt, 3);
    int layersB = sqlite3_column_int(stmt, 4);
    std::string paper = txt(5);
    int grammage = sqlite3_column_int(stmt, 6);
    int copy = sqlite3_column_int(stmt, 7);
    int staples = sqlite3_column_int(stmt, 8);
    int cut = sqlite3_column_int(stmt, 9);
    int fold = sqlite3_column_int(stmt, 10);
    double graphism = sqlite3_column_double(stmt, 11);
    double impos = sqlite3_column_double(stmt, 12);
    int shipping = sqlite3_column_int(stmt, 13);
    double discount = sqlite3_column_double(stmt, 14);
    double sheetP = sqlite3_column_double(stmt, 15);
    double inkP = sqlite3_column_double(stmt, 16);
    double masterP = sqlite3_column_double(stmt, 17);
    double shapingP = sqlite3_column_double(stmt, 18);
    double laborP = sqlite3_column_double(stmt, 19);
    double graphicP = sqlite3_column_double(stmt, 20);
    double imposP = sqlite3_column_double(stmt, 21);

    Risography *r = new Risography(
        desc, paper, format, copy, layersA, layersB, staples, cut, fold,
        graphism, graphism, impos, discount, shipping, inkP, sheetP, masterP,
        graphicP, shapingP, imposP, laborP, grammage);

    r->calc_quantity();
    r->rest_to_pay();
    risoPrints.push_back(r);
  }

  sqlite3_finalize(stmt);
  return risoPrints;
}

void Database::print_risography(sqlite3_int64 document_id, PrintMode mode) {
  std::vector<Risography *> prints = get_risography_of(document_id);

  for (size_t i = 0; i < prints.size(); i++) {
    if (mode == COMPLETE) {
      std::cout << prints[i]->generate_stream();
    } else {
      std::cout << "    Riso print : " << prints[i]->description << " ("
                << prints[i]->rest_to_pay() << "€)\n";
    }
    delete prints[i];
  }
}

bool Database::print_document(sqlite3_int64 document_id, PrintMode mode) {
  Facture *f = get_document(document_id);

  if (!f)
    return false;
  if (mode == PREVIEW) {
    std::cout << fit(document_id, 2) << ": " << f->get_type() << " ";
    std::cout << f->number << " - " << f->date;
    std::cout << "\t" << f->client->get_name();
    std::cout << " =" << f->get_total() << "€\n";
    for (auto it : f->riso_prints) {
      std::cout << "    riso: " << it->description;
      std::cout << " (" << it->rest_to_pay() << "€)\n";
    }
    for (auto line : f->customs) {
      std::cout << "    custom: " << line.first;
      std::cout << " " << line.second.first * line.second.second << "€\n";
    }
  } else {
    f->generate_stream();
  }
  delete f;
  return true;
}
#include <optional>

void Database::print_all_documents(PrintMode mode,
                                   std::optional<Facture::TYPE> type_filter) {
  sqlite3_stmt *stmt = nullptr;

  if (!type_filter.has_value()) {
    const char *sql = "SELECT id FROM documents ORDER BY id;";
    sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  } else {
    // convert enum to string for SQLite
    std::string type_str;
    switch (type_filter.value()) {
    case Facture::TYPE::FACTURE:
      type_str = "facture";
      break;
    case Facture::TYPE::DEVIS:
      type_str = "devis";
      break;
    case Facture::TYPE::TICKET:
      type_str = "ticket";
      break;
    }
    const char *sql = "SELECT id FROM documents WHERE type = ? ORDER BY id;";
    sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, type_str.c_str(), -1, SQLITE_TRANSIENT);
  }

  std::vector<sqlite3_int64> ids;
  while (sqlite3_step(stmt) == SQLITE_ROW)
    ids.push_back(sqlite3_column_int64(stmt, 0));
  sqlite3_finalize(stmt);

  for (sqlite3_int64 id : ids) {
    std::cout << "------------------------\n";
    print_document(id, mode);
  }
}

// void Database::print_all_documents(PrintMode mode) {
// 	std::cout << "Print all documents\n";
//     const char* sql = "SELECT id FROM documents ORDER BY id;";
//     sqlite3_stmt* stmt = nullptr;
//     sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
//
//     // Collect all ids first, close the statement, THEN print
//     std::vector<sqlite3_int64> ids;
//     while (sqlite3_step(stmt) == SQLITE_ROW)
//         ids.push_back(sqlite3_column_int64(stmt, 0));
//     sqlite3_finalize(stmt); // ← closed before any nested query
//
//     for (sqlite3_int64 id : ids) {
// 		std::cout << "------------------------\n";
//         print_document(id, mode);
// 	}
// }

// bool Database::update_document_number(sqlite3_int64 document_id,
//                                 const std::string& number) {
//     const char* sql = R"(
//         UPDATE documents
//         SET numero   = ?,
//         WHERE id = ?;
//     )";
//
//     sqlite3_stmt* stmt = nullptr;
//     sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
//
//     sqlite3_bind_text (stmt, 1, number.c_str(),   -1, SQLITE_TRANSIENT);
//     sqlite3_bind_int64(stmt, 2, document_id);
//
//     bool ok = sqlite3_step(stmt) == SQLITE_DONE;
//     sqlite3_finalize(stmt);
//     return ok;
// }
//
bool Database::update_document(sqlite3_int64 document_id, Facture::TYPE type,
                               Facture::STATUS status,
                               const std::string &notes) {
  // First, fetch the current numero to rebuild it with the new suffix
  const char *get_sql = "SELECT numero FROM documents WHERE id = ?;";
  sqlite3_stmt *get_stmt = nullptr;
  sqlite3_prepare_v2(_db, get_sql, -1, &get_stmt, nullptr);
  sqlite3_bind_int64(get_stmt, 1, document_id);

  std::string numero;
  if (sqlite3_step(get_stmt) == SQLITE_ROW) {
    auto *t = sqlite3_column_text(get_stmt, 0);
    numero = t ? reinterpret_cast<const char *>(t) : "";
  } else {
    std::cerr << "update_document: document id=" << document_id
              << " not found\n";
    sqlite3_finalize(get_stmt);
    return false;
  }
  sqlite3_finalize(get_stmt);

  // Replace last character with the new type suffix
  char suffix;
  std::string type_str, status_str;
  switch (type) {
  case Facture::TYPE::DEVIS:
    suffix = 'D';
    type_str = "devis";
    break;
  case Facture::TYPE::FACTURE:
    suffix = 'F';
    type_str = "facture";
    break;
  case Facture::TYPE::TICKET:
    suffix = 'T';
    type_str = "ticket";
    break;
  }
  switch (status) {
  case Facture::STATUS::DRAFT:
    status_str = "draft";
  case Facture::STATUS::SENT:
    status_str = "sent";
  case Facture::STATUS::CONVERTED:
    status_str = "converted";
  case Facture::STATUS::APPROVED:
    status_str = "approved";
  }

  if (!numero.empty())
    numero.back() = suffix; // 260411-045E → 260411-045F

  // Update all fields including the new numero
  const char *sql = R"(
        UPDATE documents
        SET type   = ?,
            numero = ?,
            statut = ?,
            notes  = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);

  sqlite3_bind_text(stmt, 1, type_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, numero.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, status_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 5, document_id);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok)
    std::cerr << "update_document failed: " << sqlite3_errmsg(_db) << '\n';
  else
    std::cout << "updated document id=" << document_id
              << " new numero=" << numero << '\n';

  sqlite3_finalize(stmt);
  return ok;
}

bool Database::delete_document(sqlite3_int64 document_id) {
  const char *sql = "DELETE FROM documents WHERE id = ?;";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "delete_document prepare failed: " << sqlite3_errmsg(_db)
              << '\n';
    return false;
  }

  sqlite3_bind_int64(stmt, 1, document_id);

  bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok)
    std::cerr << "delete_document failed: " << sqlite3_errmsg(_db) << '\n';
  else if (sqlite3_changes(_db) == 0)
    std::cerr << "delete_document: no document found with id=" << document_id
              << '\n';
  else
    std::cout << "delete_document: deleted document id=" << document_id << '\n';

  sqlite3_finalize(stmt);
  return ok;
}
// bool Database::update_document(sqlite3_int64 document_id,
//                                 Facture::TYPE type,
//                                 const std::string& statut,
//                                 const std::string& notes) {
//     const char* sql = R"(
//         UPDATE documents
//         SET type   = ?,
//             statut = ?,
//             notes  = ?
//         WHERE id = ?;
//     )";
//
// 	std::string t, number;
//
// 	switch(type) {
// 		case Facture::TYPE::DEVIS: t = "devis"; break;
// 		case Facture::TYPE::FACTURE: t = "facture"; break;
// 		case Facture::TYPE::TICKET: t = "ticket"; break;
// 	}
//     sqlite3_stmt* stmt = nullptr;
//     sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
//
//     sqlite3_bind_text (stmt, 1, t.c_str(),   -1, SQLITE_TRANSIENT);
//     sqlite3_bind_text (stmt, 2, statut.c_str(), -1, SQLITE_TRANSIENT);
//     sqlite3_bind_text (stmt, 3, notes.c_str(),  -1, SQLITE_TRANSIENT);
//     sqlite3_bind_int64(stmt, 4, document_id);
//
//     bool ok = sqlite3_step(stmt) == SQLITE_DONE;
//     sqlite3_finalize(stmt);
//
//     return ok;
// }

#include <curl/curl.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
struct MailPayload {
    std::string data;
    size_t pos = 0;
};

static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp) {
    auto* payload = reinterpret_cast<MailPayload*>(userp);
    size_t available = payload->data.size() - payload->pos;
    size_t to_copy   = std::min(size * nmemb, available);
    if (to_copy == 0) return 0;
    memcpy(ptr, payload->data.c_str() + payload->pos, to_copy);
    payload->pos += to_copy;
    return to_copy;
}

// Read a binary file into a vector of bytes
static std::vector<unsigned char> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open file: " + path);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

// Base64 encode binary data
static std::string base64_encode(const std::vector<unsigned char>& data) {
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int i = 0;
    unsigned char buf[3];
    int line = 0;
    for (unsigned char byte : data) {
        buf[i++] = byte;
        if (i == 3) {
            out += table[ buf[0] >> 2];
            out += table[(buf[0] & 3)  << 4 | buf[1] >> 4];
            out += table[(buf[1] & 15) << 2 | buf[2] >> 6];
            out += table[ buf[2] & 63];
            i = 0;
            if ((line += 4) >= 76) { out += "\r\n"; line = 0; }
        }
    }
    if (i > 0) {
        buf[i] = buf[i+1] = 0;
        out += table[ buf[0] >> 2];
        out += table[(buf[0] & 3)  << 4 | buf[1] >> 4];
        out += (i > 1) ? table[(buf[1] & 15) << 2] : '=';
        out += '=';
    }
    return out;
}

bool send_email_smtp(const std::string& to,
                     const std::string& subject,
                     const std::string& body,
                     const std::string& attachment_path = "",  // ← new
                     const std::string& from     = "atelier@example.com",
                     const std::string& smtp_url = "smtps://smtp.gmail.com:465",
                     const std::string& user     = "atelier@gmail.com",
                     const std::string& password = "your_app_password") {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    const std::string boundary = "----=_boundary_hazardous_collective";

    // Extract filename from path  "data/260413-045D-test.png" → "260413-045D-test.png"
    std::string attachment_name = attachment_path;
    auto slash = attachment_path.rfind('/');
    if (slash != std::string::npos)
        attachment_name = attachment_path.substr(slash + 1);

    // Build MIME multipart body
    std::ostringstream mail;
    mail << "To: "       << to      << "\r\n"
         << "From: "     << from    << "\r\n"
         << "Subject: "  << subject << "\r\n"
         << "MIME-Version: 1.0\r\n"
         << "Content-Type: multipart/mixed; boundary=\"" << boundary << "\"\r\n"
         << "\r\n"

         // Part 1 — plain text body
         << "--" << boundary << "\r\n"
         << "Content-Type: text/plain; charset=utf-8\r\n"
         << "\r\n"
         << body << "\r\n";

    // Part 2 — attachment (only if path provided)
    if (!attachment_path.empty()) {
        auto file_bytes = read_file(attachment_path);
        std::string b64 = base64_encode(file_bytes);

        mail << "--" << boundary << "\r\n"
             << "Content-Type: image/png\r\n"
             << "Content-Transfer-Encoding: base64\r\n"
             << "Content-Disposition: attachment; filename=\"" << attachment_name << "\"\r\n"
             << "\r\n"
             << b64 << "\r\n";
    }

    mail << "--" << boundary << "--\r\n";

    MailPayload payload;
    payload.data = mail.str();

    struct curl_slist* recipients = curl_slist_append(nullptr, to.c_str());

    curl_easy_setopt(curl, CURLOPT_URL,          smtp_url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME,      user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD,      password.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM,     ("<" + from + ">").c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT,     recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION,  read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA,      &payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD,        1L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL,       CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_VERBOSE,       0L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        std::cerr << "send_email failed: " << curl_easy_strerror(res) << '\n';

    return res == CURLE_OK;
}

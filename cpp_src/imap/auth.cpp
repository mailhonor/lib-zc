/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"
// for SCRAM-SHA-256
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

zcc_namespace_begin;

int imap_client::do_auth(const char *user, const char *password)
{
    if (need_close_connection_)
    {
        return -1;
    }

    // 根据 capability 选择认证方式
    // 优先使用 SCRAM-SHA-256（更安全），其次使用 LOGIN
    if (capability_.find("scram-sha-256") != std::string::npos)
    {
        zcc_imap_client_debug("imap 使用 SCRAM-SHA-256 认证");
        return do_auth_scram_sha256(user, password);
    }
    else
    {
        zcc_imap_client_debug("imap 使用 LOGIN 认证");
        return do_auth_login(user, password);
    }
}

int imap_client::do_auth_login(const char *user, const char *password)
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    std::string linebuf;
    std::string linebuf_debug;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("L login ").append(escape_string(user));
    linebuf_debug = linebuf;
    linebuf.append(" ").append(escape_string(password));
    fp_append(linebuf).fp_append("\r\n");
    linebuf_debug.append(" ********");
    zcc_imap_client_debug_protocol_write(linebuf_debug);
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }

        if ((r = parse_imap_result('L', response_tokens)) < 1)
        {
            if (r == 0)
            {
                zcc_imap_client_debug("imap 登录(LOGIN)失败");
            }
            return r;
        }
        if (!(response_tokens.first_line_.compare(0, 17, "L OK [CAPABILITY ")))
        {
            capability_ = response_tokens.first_line_.substr(16);
            tolower(capability_);
            capability_clear_flag_ = false;
            auth_capability_ = true;
        }
        break;
    }

    return 1;
}

static bool pbkdf2_sha256(const std::string &password, const unsigned char *salt, int saltlen, int iter, unsigned char *out, int outlen)
{
    return PKCS5_PBKDF2_HMAC(password.c_str(), (int)password.size(), salt, saltlen, iter, EVP_sha256(), outlen, out) == 1;
}

static std::string hmac_sha256(const unsigned char *key, int keylen, const unsigned char *data, int datalen)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdlen = 0;
    HMAC(EVP_sha256(), key, keylen, data, datalen, md, &mdlen);
    return std::string((char *)md, mdlen);
}

static std::string sha256_hash(const unsigned char *data, int datalen)
{
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256(data, datalen, md);
    return std::string((char *)md, SHA256_DIGEST_LENGTH);
}

int imap_client::do_auth_scram_sha256(const char *user, const char *password)
{
    if (need_close_connection_)
    {
        return -1;
    }

    // Generate client nonce
    unsigned char rnd[24];
    if (RAND_bytes(rnd, sizeof(rnd)) != 1)
    {
        zcc_imap_client_debug("RAND_bytes failed");
        return -1;
    }
    std::string clientnonce = base64_encode(rnd, sizeof(rnd));

    std::string gs2_header = "n,,";
    // client-first-message-bare
    std::string client_first_bare = "n=";
    // Note: skipping SASLprep; use raw username
    client_first_bare.append(user);
    client_first_bare.append(",r=");
    client_first_bare.append(clientnonce);

    std::string client_first_message = gs2_header + client_first_bare;

    // Start AUTHENTICATE
    std::string linebuf;
    response_tokens response_tokens;

    linebuf = "L AUTHENTICATE SCRAM-SHA-256";
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_protocol_write(linebuf);

    // Wait for server continuation ('+')
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }
        if (response_tokens.token_vector_[0] == "+")
        {
            // server may include initial challenge after '+ '
            std::string payload;
            if (response_tokens.first_line_.size() > 2)
            {
                // skip "+ " prefix
                payload = response_tokens.first_line_.substr(2);
            }

            // If server sent a challenge, process it; otherwise send client-first-message
            if (payload.empty())
            {
                std::string b64 = base64_encode(client_first_message);
                fp_append(b64).fp_append("\r\n");
                zcc_imap_client_debug_protocol_write(std::string("<scram client-first-message>"));
                continue;
            }

            // server-first-message (base64)
            std::string server_first_raw = base64_decode(payload);
            // parse server-first: r=...,s=...,i=...
            std::string r_val, s_val;
            int i_val = 0;
            size_t pos = 0;
            while (pos < server_first_raw.size())
            {
                size_t comma = server_first_raw.find(',', pos);
                std::string kv = (comma == std::string::npos) ? server_first_raw.substr(pos) : server_first_raw.substr(pos, comma - pos);
                size_t eq = kv.find('=');
                if (eq != std::string::npos)
                {
                    std::string k = kv.substr(0, eq);
                    std::string v = kv.substr(eq + 1);
                    if (k == "r")
                        r_val = v;
                    else if (k == "s")
                        s_val = v;
                    else if (k == "i")
                        i_val = atoi(v.c_str());
                }
                if (comma == std::string::npos)
                    break;
                pos = comma + 1;
            }

            if (r_val.empty() || s_val.empty() || i_val <= 0)
            {
                zcc_imap_client_debug("SCRAM server-first invalid: %s", server_first_raw.c_str());
                return 0;
            }

            // combined nonce must start with clientnonce
            if (r_val.find(clientnonce) != 0)
            {
                zcc_imap_client_debug("SCRAM nonce mismatch");
                return 0;
            }

            // decode salt
            std::string salt = base64_decode(s_val);

            // compute saltedPassword
            unsigned char salted[SHA256_DIGEST_LENGTH];
            if (!pbkdf2_sha256(password, (const unsigned char *)salt.data(), (int)salt.size(), i_val, salted, SHA256_DIGEST_LENGTH))
            {
                zcc_imap_client_debug("SCRAM PBKDF2 failed");
                return -1;
            }

            // clientKey = HMAC(saltedPassword, "Client Key")
            std::string clientKey = hmac_sha256(salted, SHA256_DIGEST_LENGTH, (const unsigned char *)"Client Key", 10);
            // storedKey = H(clientKey)
            std::string storedKey = sha256_hash((const unsigned char *)clientKey.data(), (int)clientKey.size());

            // client-final-without-proof
            std::string cbind = "c=" + base64_encode(gs2_header);
            std::string client_final_without_proof = cbind + ",r=" + r_val;

            // authMessage = client-first-bare + "," + server-first-message + "," + client-final-without-proof
            std::string authMessage = client_first_bare + "," + server_first_raw + "," + client_final_without_proof;

            // clientSignature = HMAC(storedKey, authMessage)
            std::string clientSignature = hmac_sha256((const unsigned char *)storedKey.data(), (int)storedKey.size(), (const unsigned char *)authMessage.data(), (int)authMessage.size());

            // clientProof = clientKey XOR clientSignature
            std::string clientProof;
            clientProof.resize(clientKey.size());
            for (size_t k = 0; k < clientKey.size(); k++)
            {
                clientProof[k] = clientKey[k] ^ clientSignature[k];
            }

            // serverKey = HMAC(saltedPassword, "Server Key")
            std::string serverKey = hmac_sha256(salted, SHA256_DIGEST_LENGTH, (const unsigned char *)"Server Key", 10);

            // final message
            std::string client_final_message = client_final_without_proof + ",p=" + base64_encode(clientProof);
            std::string b64_final = base64_encode(client_final_message);
            fp_append(b64_final).fp_append("\r\n");
            zcc_imap_client_debug_protocol_write(std::string("<scram client-final-message>"));

            // wait for server final response
            while (1)
            {
                zcc_imap_client_read_token_vecotr_one_loop();
                if (response_tokens.token_vector_[0] == "*")
                {
                    continue;
                }
                if (response_tokens.token_vector_[0] == "+")
                {
                    std::string payload2;
                    if (response_tokens.first_line_.size() > 2)
                        payload2 = response_tokens.first_line_.substr(2);
                    std::string server_final_raw = base64_decode(payload2);
                    // server-final-message: may contain v=...
                    size_t eq = server_final_raw.find("v=");
                    if (eq == std::string::npos)
                    {
                        zcc_imap_client_debug("SCRAM server-final missing verifier: %s", server_final_raw.c_str());
                        return 0;
                    }
                    std::string v = server_final_raw.substr(eq + 2);
                    std::string serverSignature = hmac_sha256((const unsigned char *)serverKey.data(), (int)serverKey.size(), (const unsigned char *)authMessage.data(), (int)authMessage.size());
                    std::string serverSignature_b64 = base64_encode(serverSignature);
                    if (v != serverSignature_b64)
                    {
                        zcc_imap_client_debug("SCRAM server signature mismatch");
                        return 0;
                    }
                    // now expect tagged OK
                    continue;
                }
                // tagged response
                return parse_imap_result('L', response_tokens);
            }
        }
        else
        {
            // unexpected
            return parse_imap_result('L', response_tokens);
        }
    }

    return 0;
}

zcc_namespace_end;
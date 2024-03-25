// (c) 2024. Takhir Latypov <cregennandev@gmail.com>
#include <Vault.h>
#include <sha1.h>
#include <TOTP.h>

const auto EEPROM_MARKER_0 = 0xBA;
const auto EEPROM_MARKER_1 = 0xBE;
const auto SECRET_LEFT_MARKER_0 = 0xFF;
const auto SECRET_LEFT_MARKER_1 = 0xFA;
const auto SECRET_RIGHT_MARKER_0 = 0xFA;
const auto SECRET_RIGHT_MARKER_1 = 0xFF;

std::vector<uint8_t> decryptSecretWithMarkers(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey);

bool verifySecretKey(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey);

std::vector<uint8_t> encryptSecret(const std::string & rawSecret, const std::vector<uint8_t> & secretKey);

VAULT_INIT_RESULT Vault_::Initialize(time_t last_sync_millis, time_t client_utc) {
    this->lastSyncMillis = last_sync_millis;
    this->clientUtc = client_utc;

    auto entriesCount = EEPROM.read(2);

    if (EEPROM_MARKER_0 != EEPROM.read(0)
        && EEPROM_MARKER_1 != EEPROM.read(1)
        && TOTP_KEYS_COUNT_LIMIT > entriesCount){

        entriesCount = 0; // <- default stored entries count
        EEPROM.write(0, EEPROM_MARKER_0);
        EEPROM.write(1, EEPROM_MARKER_1);
        EEPROM.write(2, entriesCount);
        EEPROM.commit();

        this->Initialized = true;

        return VAULT_INIT_RESULT::SUCCESS_NEWBORN;
    }

    auto grandOffset = 3;

    for(auto i = 0; i < entriesCount; i++){
        //Secret name
        auto nameLength = EEPROM.read(grandOffset++);
        if (nameLength > TOTP_KEY_NAME_MAX_LENGTH || nameLength == 0){
            return VAULT_INIT_RESULT::MALFORMED;
        }
        std::string name;
        name.reserve(nameLength);
        for(auto nc = 0; nc < nameLength; nc++){
            name[nc] = EEPROM.read(grandOffset++);
        }

        //Secret contents
        auto secretLength = EEPROM.read(grandOffset++);
        if (secretLength > TOTP_KEY_SECRET_MAX_LENGTH || secretLength == 0){
            return VAULT_INIT_RESULT::MALFORMED;
        }
        std::vector<uint8_t> secret;
        secret.reserve(secretLength);
        for(auto sc = 0; sc < secretLength; sc++){
            secret[sc] = EEPROM.read(grandOffset++);
        }

        //Key characters length
        auto characterLength = EEPROM.read(grandOffset++);
        VaultEntry entry;
        entry.Digits = characterLength;
        entry.Name = name;
        entry.Secret = secret;
        VaultEntries.push_back(entry);
    }
    this->Initialized = true;

    return VAULT_INIT_RESULT::SUCCESS;
}

void Vault_::ForceReset() {
    EEPROM.write(0, EEPROM_MARKER_0);
    EEPROM.write(1, EEPROM_MARKER_1);
    EEPROM.write(2, 0); // <- default stored entries count
    EEPROM.commit();
}

VAULT_ADD_ENTRY_RESULT Vault_::addEntry(const std::string & name, const std::string & rawSecret, int digitsCount = 6) {
    if (!this->Unlocked){
        return VAULT_ADD_ENTRY_RESULT::VAULT_IS_LOCKED;
    }

    if (VaultEntries.size() + 1 > TOTP_KEYS_COUNT_LIMIT){
        return VAULT_ADD_ENTRY_RESULT::NO_MORE_SPACE;
    }

    if (rawSecret.empty() || rawSecret.size() > TOTP_KEY_SECRET_MAX_LENGTH){
        return VAULT_ADD_ENTRY_RESULT::SECRET_LENGTH_EXCEEDED;
    }

    if (name.empty() || name.size() > TOTP_KEY_NAME_MAX_LENGTH){
        return VAULT_ADD_ENTRY_RESULT::NAME_LENGTH_EXCEEDED;
    }

    VaultEntry entry;
    entry.Name = name;
    entry.Digits = digitsCount;
    entry.Secret = encryptSecret(rawSecret, this->SecretKey);
    std::vector<uint8_t> rawSecretBytes(rawSecret.begin(), rawSecret.end());
    this->VaultEntries.push_back(entry);
    this->RawKeys.push_back(rawSecretBytes);
    this->burnVaultEntries();
    return VAULT_ADD_ENTRY_RESULT::SUCCESS;
}

VAULT_REMOVE_ENTRY_RESULT Vault_::removeEntry(int entryId) {
    if(!this->Unlocked){
        return VAULT_REMOVE_ENTRY_RESULT::VAULT_IS_LOCKED;
    }
    if (entryId < 0 || entryId >= VaultEntries.size()){
        return VAULT_REMOVE_ENTRY_RESULT::NOT_FOUND;
    }

    VaultEntries.erase(VaultEntries.begin() + entryId);
    RawKeys.erase(RawKeys.begin() + entryId);
    this->burnVaultEntries();
}

void Vault_::burnVaultEntries() {
    EEPROM.write(0, EEPROM_MARKER_0);
    EEPROM.write(1, EEPROM_MARKER_1);
    EEPROM.write(2, VaultEntries.size());
    auto grandIndex = 3;

    for(const auto & entry : VaultEntries){
        EEPROM.write(grandIndex++, entry.Name.size());
        for(auto& c : entry.Name){
            EEPROM.write(grandIndex++, c);
        }
        EEPROM.write(grandIndex++, entry.Secret.size());
        for(auto& c : entry.Secret){
            EEPROM.write(grandIndex++, c);
        }
        EEPROM.write(grandIndex++, entry.Digits);
    }

    EEPROM.commit();
}

VAULT_UNLOCK_RESULT Vault_::unlock(const std::string & password) {
    if (password.empty() || password.size() > TOTP_KEY_PASSWORD_MAX_LENGTH){
        return VAULT_UNLOCK_RESULT::MALFORMED_PASSWORD;
    }
    if (!this->Initialized){
        return VAULT_UNLOCK_RESULT::NOT_INITIALIZED;
    }

    Sha1.init();
    Sha1.print(password.c_str());
    auto resultPointer = Sha1.result();
    std::vector<uint8_t> secretKey(resultPointer, resultPointer + 20);

    if (this->VaultEntries.empty()){
        this->Unlocked = true;
        this->SecretKey = secretKey;
        return VAULT_UNLOCK_RESULT::SUCCESS;
    }

    if (verifySecretKey(this->VaultEntries[0].Secret, secretKey)){
        this->Unlocked = true;
        this->SecretKey = secretKey;
        return VAULT_UNLOCK_RESULT::SUCCESS;
    }else{
        return VAULT_UNLOCK_RESULT::INVALID_PASSWORD;
    }
}

std::pair<VAULT_GET_KEY_RESULT, std::string> Vault_::getKey(int entryId) {
    if (!this->Initialized){
        return std::make_pair(VAULT_GET_KEY_RESULT::VAULT_NOT_INITIALIZED,std::string());
    }
    if (!this->Unlocked){
        return std::make_pair(VAULT_GET_KEY_RESULT::VAULT_IS_LOCKED,std::string());
    }
    if (this->VaultEntries.empty() || entryId >= VaultEntries.size() || entryId <= 0){
        return std::make_pair(VAULT_GET_KEY_RESULT::NOT_FOUND, std::string());
    }

    TOTP totp(&this->SecretKey.front(), (int)this->SecretKey.size());
    auto code = totp.getCode(this->clientUtc + (long)((millis() - this->lastSyncMillis) / 60));
    return std::make_pair(VAULT_GET_KEY_RESULT::SUCCESS, std::string(code, code + 6));
}

std::vector<uint8_t> encryptSecret(const std::string & rawSecret, const std::vector<uint8_t> & secretKey) {
    std::vector<uint8_t> result;
    result.reserve(rawSecret.size() + 4);
    result[0] = SECRET_LEFT_MARKER_0 ^ secretKey[0];
    result[1] = SECRET_LEFT_MARKER_1 ^ secretKey[1];

    auto secretKeySize = secretKey.size();
    auto rawSecretSize = rawSecret.size();
    auto secretKeyIndex = 2;

    for(auto i = 0; i < rawSecretSize; i++ ){
        result[i + 2] = rawSecret[i] ^ secretKey[secretKeyIndex];
        secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeySize;
    }
    result[rawSecretSize + 2] = SECRET_RIGHT_MARKER_0 ^ secretKey[secretKeyIndex];
    secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeySize;
    result[rawSecretSize + 3] = SECRET_RIGHT_MARKER_1 ^ secretKey[secretKeyIndex];

    return result;
}

std::vector<uint8_t> decryptSecretWithMarkers(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey){
    std::vector<uint8_t> result;
    result.reserve(encryptedSecret.size());
    auto secretKeyIndex = 0;
    auto secretKeyLength = secretKey.size();

    for(auto i = 0; i < encryptedSecret.size(); i++){
        result[i] = encryptedSecret[i] ^ secretKey[secretKeyIndex];
        secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeyLength;
    }

    return result;
}

bool verifySecretKey(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey){
    auto decrypted = decryptSecretWithMarkers(encryptedSecret, secretKey);
    auto size = decrypted.size();
    return size > 4
            && decrypted[0] == SECRET_LEFT_MARKER_0
            && decrypted[1] == SECRET_LEFT_MARKER_1
            && decrypted[size - 2] == SECRET_RIGHT_MARKER_0
            && decrypted[size - 1] == SECRET_RIGHT_MARKER_1;
}
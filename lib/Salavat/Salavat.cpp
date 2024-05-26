// (c) 2024. Takhir Latypov <cregennandev@gmail.com>
#include <Salavat.h>
#include <sha1.h>
#include <TOTP.h>
#include "FlashStorage_SAMD.h"
#include "Warlin.h"
#include "Base32.h"

const auto EEPROM_MARKER_0 = 0xBA;
const auto EEPROM_MARKER_1 = 0xBE;
const auto SECRET_LEFT_MARKER_0 = 0xFF;
const auto SECRET_LEFT_MARKER_1 = 0xFA;
const auto SECRET_RIGHT_MARKER_0 = 0xFA;
const auto SECRET_RIGHT_MARKER_1 = 0xFF;

std::vector<uint8_t> decryptSecretWithMarkers(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey);

bool verifySecretKey(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey);

std::vector<uint8_t> encryptSecret(const std::string & rawSecretString, const std::vector<uint8_t> & secretKey);

std::vector<uint8_t> decryptWithMasterKey(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & masterPassword);

VAULT_INIT_RESULT Salavat_::Initialize() {
    if (this->VaultInitialized){
        return VAULT_INIT_RESULT::ALREADY_INITIALIZED;
    }

    auto entriesCount = EEPROM.read(2);

    if (EEPROM_MARKER_0 != EEPROM.read(0)
        || EEPROM_MARKER_1 != EEPROM.read(1)
        || entriesCount > TOTP_KEYS_COUNT_LIMIT){

        ForceReset();

        this->VaultInitialized = true;

        return VAULT_INIT_RESULT::SUCCESS_NEWBORN;
    }

    auto grandOffset = 3;

    SendDebugMessage("ENTRIES FOUND: ", std::to_string(entriesCount).c_str());
    VaultEntries.clear();

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
        if (secretLength > TOTP_KEY_SECRET_MAX_LENGTH || secretLength <= 0){
            SendDebugMessage("Secret contents malformed, secret length: ", std::to_string(secretLength).c_str());
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
    this->VaultInitialized = true;

    return VAULT_INIT_RESULT::SUCCESS;
}

void Salavat_::ForceReset() {
    EEPROM.write(0, EEPROM_MARKER_0);
    EEPROM.write(1, EEPROM_MARKER_1);
    EEPROM.write(2, 0); // <- default stored entries count
    EEPROM.commit();
}

VAULT_ADD_ENTRY_RESULT Salavat_::addEntry(const std::string & name, const std::string & rawSecret, int digitsCount = 6) {
    if (!this->VaultUnlocked){
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
    entry.Secret = encryptSecret(rawSecret, this->MasterPasswordHash);

    auto rawSecretBytes = decodeBase32Secret(rawSecret);
    this->VaultEntries.push_back(entry);
    this->UnencryptedSecrets.push_back(rawSecretBytes);
    this->burnVaultEntries();
    return VAULT_ADD_ENTRY_RESULT::SUCCESS;
}

VAULT_REMOVE_ENTRY_RESULT Salavat_::removeEntry(int entryId) {
    if(!this->VaultUnlocked){
        return VAULT_REMOVE_ENTRY_RESULT::VAULT_IS_LOCKED;
    }
    if (entryId < 0 || entryId >= VaultEntries.size()){
        return VAULT_REMOVE_ENTRY_RESULT::NOT_FOUND;
    }

    VaultEntries.erase(VaultEntries.begin() + entryId);
    UnencryptedSecrets.erase(UnencryptedSecrets.begin() + entryId);
    this->burnVaultEntries();

    return VAULT_REMOVE_ENTRY_RESULT::SUCCESS;
}

void Salavat_::burnVaultEntries() {
    EEPROM.write(0, EEPROM_MARKER_0);
    EEPROM.write(1, EEPROM_MARKER_1);
    SendDebugMessage("Burning entries length: ", std::to_string(VaultEntries.size()).c_str());
    EEPROM.write(2, VaultEntries.size());
    auto grandIndex = 3;

    for(const auto & entry : VaultEntries){
        SendDebugMessage("Burning name length: ", std::to_string(entry.Name.size()).c_str());
        EEPROM.write(grandIndex++, entry.Name.size());
        for(auto& c : entry.Name){
            SendDebugMessage("Burning name byte: ", std::to_string(c).c_str());
            EEPROM.write(grandIndex++, c);
        }

        SendDebugMessage("Burning secret length: ", std::to_string(entry.Secret.size()).c_str());
        EEPROM.write(grandIndex++, entry.Secret.size());
        for(auto& c : entry.Secret){
            SendDebugMessage("Burning secret byte: ", std::to_string(c).c_str());
            EEPROM.write(grandIndex++, c);
        }

        SendDebugMessage("Burning digits: ", std::to_string(entry.Digits).c_str());
        EEPROM.write(grandIndex++, entry.Digits);
    }

    EEPROM.commit();
}

VAULT_UNLOCK_RESULT Salavat_::unlock(const std::string & password) {
    if (password.empty() || password.size() > TOTP_KEY_PASSWORD_MAX_LENGTH){
        return VAULT_UNLOCK_RESULT::MALFORMED_PASSWORD;
    }
    if (!this->VaultInitialized){
        return VAULT_UNLOCK_RESULT::NOT_INITIALIZED;
    }

    Sha1.init();
    Sha1.print(password.c_str());
    auto hashPointer = Sha1.result();
    std::vector<uint8_t> passwordHash(hashPointer, hashPointer + 20);

    if (this->VaultEntries.empty()){
        this->VaultUnlocked = true;
        this->MasterPasswordHash = passwordHash;
        return VAULT_UNLOCK_RESULT::SUCCESS;
    }

    if (verifySecretKey(this->VaultEntries[0].Secret, passwordHash)){
        this->VaultUnlocked = true;
        this->MasterPasswordHash = passwordHash;
        this->UnencryptedSecrets.clear();

        for(auto& entry : this->VaultEntries){
            this->UnencryptedSecrets.push_back(decryptWithMasterKey(entry.Secret, passwordHash));
        }

        return VAULT_UNLOCK_RESULT::SUCCESS;
    }else{
        return VAULT_UNLOCK_RESULT::INVALID_PASSWORD;
    }
}

std::pair<VAULT_GET_KEY_RESULT, std::string> Salavat_::getKey(int entryId, long currentUtc) {
    if (!this->VaultInitialized){
        return std::make_pair(VAULT_GET_KEY_RESULT::VAULT_NOT_INITIALIZED,std::string());
    }
    if (!this->VaultUnlocked){
        return std::make_pair(VAULT_GET_KEY_RESULT::VAULT_IS_LOCKED,std::string());
    }
    if (this->VaultEntries.empty() || entryId >= VaultEntries.size() || entryId < 0){
        return std::make_pair(VAULT_GET_KEY_RESULT::NOT_FOUND, std::string());
    }

    auto& unencryptedSecret = this->UnencryptedSecrets[entryId];
    auto rawSecret = std::string(unencryptedSecret.begin(), unencryptedSecret.end());

    SendDebugMessage("Current UTC", std::to_string(currentUtc).c_str());
    SendDebugMessage("Current secret", rawSecret.c_str());
    SendDebugMessage("Unencrypted secret length", std::to_string(unencryptedSecret.size()).c_str());

    TOTP totp(&unencryptedSecret.front(), (int)unencryptedSecret.size());
    auto code = totp.getCode(currentUtc);

    return std::make_pair(VAULT_GET_KEY_RESULT::SUCCESS, std::string(code, code + 6));
}

std::vector<uint8_t> Salavat_::_service_read_eeprom_header() {
    std::vector<uint8_t> t{};
    t.push_back(EEPROM.read(0));
    t.push_back(EEPROM.read(1));
    t.push_back(EEPROM.read(2));
    return t;
}

std::size_t Salavat_::secretsCount() {
    return this->VaultEntries.size();
}

std::vector<std::string> Salavat_::getEntryNames() {
    std::vector<std::string> acc;
    acc.reserve(this->VaultEntries.size()); // reserve для оптимизации push_back. Не путать с resize
    for (const auto &item: this->VaultEntries){
        acc.push_back(item.Name);
    }
    return acc;
}

std::vector<uint8_t> encryptSecret(const std::string & rawSecret, const std::vector<uint8_t> & secretKey) {
    std::vector<uint8_t> result;
    auto rawSecretDecoded = decodeBase32Secret(rawSecret);
    result.resize(rawSecretDecoded.size() + 4);
    result[0] = SECRET_LEFT_MARKER_0 ^ secretKey[0];
    result[1] = SECRET_LEFT_MARKER_1 ^ secretKey[1];

#ifdef KEECHAIN_DEBUG_ENABLED
    auto hex = vectorToHex(rawSecretDecoded);
    SendDebugMessage("Raw secret: ", hex.c_str());
#endif

    SendDebugMessage("Secret key: ", std::string(secretKey.begin(), secretKey.end()).c_str());

    auto secretKeySize = secretKey.size();
    auto rawSecretSize = rawSecretDecoded.size();
    auto secretKeyIndex = 2;

    for(auto i = 0; i < rawSecretSize; i++ ){
        result[i + 2] = rawSecretDecoded[i] ^ secretKey[secretKeyIndex];
        secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeySize;
    }
    result[rawSecretSize + 2] = SECRET_RIGHT_MARKER_0 ^ secretKey[secretKeyIndex];
    secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeySize;
    result[rawSecretSize + 3] = SECRET_RIGHT_MARKER_1 ^ secretKey[secretKeyIndex];

    SendDebugMessage("Result length: ", std::to_string(result.size()).c_str());

    return result;
}

std::vector<uint8_t> decryptSecretWithMarkers(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & secretKey){
    std::vector<uint8_t> result;
    result.resize(encryptedSecret.size());
    auto secretKeyIndex = 0;
    auto secretKeyLength = secretKey.size();

    for(auto i = 0; i < encryptedSecret.size(); i++){
        result[i] = encryptedSecret[i] ^ secretKey[secretKeyIndex];
        secretKeyIndex = (secretKeyIndex + 1) % (int)secretKeyLength;
    }

    return result;
}

std::vector<uint8_t> decryptWithMasterKey(const std::vector<uint8_t> & encryptedSecret, const std::vector<uint8_t> & masterPassword){
    auto raw = decryptSecretWithMarkers(encryptedSecret, masterPassword);
    return std::vector<uint8_t>(raw.begin() + 2, raw.end() - 2);
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

std::vector<uint8_t> decodeBase32Secret(std::string secret) {
    Base32 base32;
    byte* secretBytesPointer;

    auto length = base32.fromBase32(const_cast<byte *>(reinterpret_cast<const byte *>(secret.c_str())), secret.size(), secretBytesPointer);
    return std::vector<uint8_t>(secretBytesPointer, secretBytesPointer + length);
}

std::string vectorToHex(std::vector<uint8_t> &vector) {
    static const auto hex = "0123456789ABCDEF";
    std::string str;
    auto pointer = vector.data();
    auto length = vector.size();

    for(auto i = 0; i < length; i++){
        str += (char)hex[pointer[i] / 16];
        str += (char)hex[pointer[i] % 16];
        str += " ";
    }

    return str;
}

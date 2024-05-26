// (C) 2024 Takhir Latypov <cregennandev@gmail.com>
// MIT License

#ifndef KEECHAIN_SALAVAT_H_GUARD
#define KEECHAIN_SALAVAT_H_GUARD
#pragma once
#include <EnumReflection.h>
#include <string>
#include <vector>

constexpr auto TOTP_KEYS_COUNT_LIMIT = 5;
constexpr auto TOTP_KEY_NAME_MAX_LENGTH = 20;
constexpr auto TOTP_KEY_SECRET_MAX_LENGTH = 50;
constexpr auto TOTP_KEY_PASSWORD_MAX_LENGTH = 30;

Z_ENUM_NS(
    VAULT_UNLOCK_RESULT,
    SUCCESS,
    MALFORMED_PASSWORD,
    INVALID_PASSWORD,
    NOT_INITIALIZED
)

Z_ENUM_NS(
    VAULT_ADD_ENTRY_RESULT,
    SUCCESS,
    VAULT_IS_LOCKED,
    NAME_LENGTH_EXCEEDED,
    SECRET_LENGTH_EXCEEDED,
    NO_MORE_SPACE
)

Z_ENUM_NS(
    VAULT_INIT_RESULT,
    SUCCESS,
    SUCCESS_NEWBORN,
    MALFORMED,
    ALREADY_INITIALIZED
)

Z_ENUM_NS(
    VAULT_REMOVE_ENTRY_RESULT,
    SUCCESS,
    VAULT_IS_LOCKED,
    NOT_FOUND
)

Z_ENUM_NS(
    VAULT_GET_KEY_RESULT,
    VAULT_NOT_INITIALIZED,
    VAULT_IS_LOCKED,
    NOT_FOUND,
    SUCCESS)

struct VaultEntry
{
    std::string Name;
    std::vector<uint8_t> Secret;
    int Digits;
};

class Salavat_{
public:
    VAULT_ADD_ENTRY_RESULT addEntry(const std::string & name, const std::string & rawSecret, int digitsCount);
    VAULT_REMOVE_ENTRY_RESULT removeEntry(int entryId);
    std::pair<VAULT_GET_KEY_RESULT, std::string> getKey(int entryId, long currentUtc);
    void ForceReset();
    VAULT_INIT_RESULT Initialize();
    VAULT_UNLOCK_RESULT unlock(const std::string & password);
    std::vector<uint8_t> _service_read_eeprom_header();
    std::size_t secretsCount();
    std::vector<std::string> getEntryNames();
private:
    /*
     * Прожиг состояния хранилища на плату
     * Использовать с осторожностью! Тратит ресурс микросхемы памяти
     */
    void burnVaultEntries();

    std::vector<VaultEntry> VaultEntries;

    std::vector<uint8_t> MasterPasswordHash;

    std::vector<std::vector<uint8_t>> UnencryptedSecrets;

    bool VaultUnlocked = false;

    //Выполнена ли инициализация хранилища
    bool VaultInitialized = false;
};

std::vector<uint8_t> decodeBase32Secret(std::string secret);

std::string vectorToHex(std::vector<uint8_t> & vector);


#endif //KEECHAIN_SALAVAT_H_GUARD

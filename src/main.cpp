#include "main.h"
#include "TOTP.h"

/*
 * Обработчик DISCOVER
 * Аргументов нет
 * Отвечает ACK
 */
void discoverHandler(std::deque<std::string> & params)
{
   Warlin.writeLine(NameOf(PROTOCOL_RESPONSE_TYPE::ACK));
}

/*
 * Обработчик для SYNC
 * Аргументов нет
 * Отвечает SYNCR + int количество ключей в памяти
 */
void syncHandler(std::deque<std::string> & params)
{
    auto result = Salavat.Initialize();

    if (result == VAULT_INIT_RESULT::MALFORMED){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_INIT_MALFORMED });
        return;
    }

    Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::SYNCR), std::to_string(Salavat.secretsCount()) });
}

/*
 * Обработчик для UNLOCK
 * Аргумент: string мастер-пароль
 * Отвечает ACK
 */
void unlockHandler(std::deque<std::string> & params){
    if (params.size() < 1){
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_NOT_ENOUGH_PARAMS});
        return;
    }

    auto password = params[0];
    auto unlockResult = Salavat.unlock(password);
    if (unlockResult != VAULT_UNLOCK_RESULT::SUCCESS){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), NameOf(unlockResult) });
        return;
    }

    Warlin.writeLine(NameOf(PROTOCOL_RESPONSE_TYPE::ACK));
}

/*
 * Обработчик для SERVICE_TRY_READ_EEPROM
 * Аргументов нет
 * Возвращает SERVICE
 */
void serviceEEPROMHandler(std::deque<std::string> & params){
    auto t = Salavat._service_read_eeprom_header();
    SendDebugMessage("EEPROM READ COMPLETED, COUNT: ", std::to_string(t.size()).c_str());
    std::string result;

    for (const auto &item: t){
        result.append(std::to_string(item));
        result.append(" ");
    }
    SendDebugMessage(result.c_str());

    Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::SERVICE)});
}

/*
 * Обработчик GET_ENTRIES
 * Аргументов нет
 * Возвращает ENTRIES
 * - string[] названия ключей
 */
void getStoredNamesHandler(std::deque<std::string> & params){
    auto names = Salavat.getEntryNames();
    Warlin.writeLine(PROTOCOL_RESPONSE_TYPE::ENTRIES, names);
}

/*
 * Обработчик STORE_ENTRY
 * Аргументы:
 * - string название
 * - string base32-кодированный секрет в верхнем регистре
 * - int количество цифр (UNUSED)
 * Возвращает ACK
 */
void storeSecretHandler(std::deque<std::string> & params){
    if (params.size() < 3){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_NOT_ENOUGH_PARAMS });
        return;
    }
    auto digits = std::stoi(params[2]);
    auto result = Salavat.addEntry(params[0], params[1], digits);

    if (result != VAULT_ADD_ENTRY_RESULT::SUCCESS){
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), NameOf(result)});
        return;
    }

    Warlin.writeLine(PROTOCOL_RESPONSE_TYPE::ACK);
}

/*
 * Обработчик для GENERATE
 * Аргументы:
 * - int индекс ключа
 * - long текущая метка UNIX
 * Возвращает OTP
 * - string одноразовый код
 */
void generateHandler(std::deque<std::string> &params) {

    if (params.size() < 2){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_NOT_ENOUGH_PARAMS });
        return;
    }

    auto index = std::stoi(params[0]);
    auto currentUtc = std::stol(params[1]);

    auto result = Salavat.getKey(index, currentUtc);
    auto status = result.first;
    auto code = result.second;

    if (status != VAULT_GET_KEY_RESULT::SUCCESS){

        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), NameOf(status)});
        return;
    }

    Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::OTP), code});
}

/*
 * Обработчик для TEST_EXPLICIT_CODE
 * Явно генерирует код аутентификации, используется для тестирования
 * Аргументы:
 * - string base32-кодированная строка секрета
 * - long текущее UTC время
 * Возвращает OTP
 * - string одноразовый код
 */
void testGenerateOTPByExplicitSecret(std::deque<std::string> &params) {
    if (params.size() < 2) {
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_NOT_ENOUGH_PARAMS});
        return;
    }

    auto secret = params[0];
    auto currentUtc = std::stol(params[1]);

    auto decoded = decodeBase32Secret(secret);
    auto decodedPointer = decoded.data();
    auto decodedLength = decoded.size();

#ifdef KEECHAIN_DEBUG_ENABLED
    SendDebugMessage("Bytes parsed from secret key: ", vectorToHex(decoded).c_str());
#endif

    TOTP totp(decodedPointer, decodedLength);

    auto code = totp.getCode(currentUtc);

    Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::OTP), std::string(code)});
}

/*
 * Обработчик для REMOVE_ENTRY
 * Аргументы:
 * - int индекс секрета
 * Возвращает ACK
 */
void removeEntryHandler(std::deque<std::string> &params){
    if (params.size() < 1) {
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), ANSWER_NOT_ENOUGH_PARAMS});
        return;
    }

    auto index = std::stoi(params[0]);

    auto result = Salavat.removeEntry(index);
    if (result == VAULT_REMOVE_ENTRY_RESULT::SUCCESS){
        Warlin.writeLine(NameOf(PROTOCOL_RESPONSE_TYPE::ACK));
        return;
    }

    Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), NameOf(result)});
}

//WARLIN<PART>DISCOVER
//WARLIN<PART>SYNC
//WARLIN<PART>UNLOCK<PART>123
//WARLIN<PART>STORE_ENTRY<PART>Google<PART>JBSWY3DPEHPK3PXP<PART>6
//WARLIN<PART>GET_ENTRIES
//WARLIN<PART>GENERATE<PART>0<PART>1716740958
//WARLIN<PART>TEST_EXPLICIT_CODE<PART>JBSWY3DPEHPK3PXP<PART>1716740851
//WARLIN<PART>REMOVE_ENTRY<PART>0

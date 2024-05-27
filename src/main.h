//
// Created by Cregennan on 27.05.2024.
//

#ifndef KEECHAIN_EMBEDDED_MAIN_H
#define KEECHAIN_EMBEDDED_MAIN_H

void discoverHandler(std::deque<std::string> & params);
void syncHandler(std::deque<std::string> & params);
void serviceEEPROMHandler(std::deque<std::string> & params);
void unlockHandler(std::deque<std::string> & params);
void getStoredNamesHandler(std::deque<std::string> & params);
void storeSecretHandler(std::deque<std::string> & params);
void generateHandler(std::deque<std::string> & params);
void testGenerateOTPByExplicitSecret(std::deque<std::string> & params);
void removeEntryHandler(std::deque<std::string> & params);

Warlin_ Warlin;
Salavat_ Salavat;

void setup() {
    Serial.begin(DEFAULT_BAUDRATE);
    while(!Serial)
    {

    }

    Warlin.bind(PROTOCOL_REQUEST_TYPE::DISCOVER, discoverHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::SYNC, syncHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::SERVICE_TRY_READ_EEPROM, serviceEEPROMHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::UNLOCK, unlockHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::GET_ENTRIES, getStoredNamesHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::STORE_ENTRY, storeSecretHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::GENERATE, generateHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::TEST_EXPLICIT_CODE, testGenerateOTPByExplicitSecret);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::REMOVE_ENTRY, removeEntryHandler);
}

void loop() {
    if (Warlin.available())
    {
        Warlin.process();
    }

    delay(100);
}

#define

#endif //KEECHAIN_EMBEDDED_MAIN_H

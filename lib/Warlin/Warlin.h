// (c) 2024 Takhir Latypov <cregennandev@gmail.com>
// MIT License

#ifndef KEECHAIN_WARLIN_H_GUARD
#define KEECHAIN_WARLIN_H_GUARD
#pragma once

#include <Arduino.h>
#include <deque>
#include <unordered_map>
#include <EnumReflection.h>
#include <vector>

#define KEECHAIN_DEBUG_ENABLED // Закомментировать в продакшене

static constexpr auto PROTOCOL_MAGIC_BEGIN = "WARLIN";

static constexpr auto PROTOCOL_DEBUG_BEGIN = "DEVICE_DEBUG";

static constexpr auto PROTOCOL_ERROR_BEGIN = "WARLIN_ERROR";

static constexpr auto DEFAULT_DELIMITER = "<PART>";

static constexpr auto DEFAULT_BAUDRATE = 115600;

Z_ENUM_NS(
    PROTOCOL_REQUEST_TYPE,
    DISCOVER,
    SYNC,
    UNLOCK,
    GET_ENTRIES,
    STORE_ENTRY,
    REMOVE_ENTRY,
    GENERATE,
    TEST_EXPLICIT_CODE,
    SERVICE_TRY_READ_EEPROM
);

Z_ENUM_NS(
    PROTOCOL_RESPONSE_TYPE,
    ACK,
    SYNCR,
    SERVICE,
    ENTRIES,
    OTP,
    ERROR
);

class Warlin_
{
    public:
        void bind(PROTOCOL_REQUEST_TYPE, void(*)(std::deque<std::string>&));
        bool available();
        void process();
        Warlin_();
        void writeLine(const std::string & str);
        void writeLine(const std::initializer_list<std::string> & args);
        void writeLine(PROTOCOL_RESPONSE_TYPE type, std::vector<std::string> & params);
        void writeLine(PROTOCOL_RESPONSE_TYPE type);
    private:
        std::unordered_map<PROTOCOL_REQUEST_TYPE, void(*)(std::deque<std::string>&)> listeners{};
};

void SendDebugMessage(const char * message);

void SendDebugMessage(const char * part1, const char * part2);

void SendErrorMessage(const char * message);

void SendErrorMessage(const char * part1, const char * part2);

#endif // Guard
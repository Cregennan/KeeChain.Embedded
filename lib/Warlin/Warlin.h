// (c) 2024 Takhir Latypov <cregennandev@gmail.com>
// MIT License

#ifndef KEECHAIN_WARLIN_H_GUARD
#define KEECHAIN_WARLIN_H_GUARD
#pragma once

#include <Arduino.h>
#include <deque>
#include <unordered_map>
#include <EnumReflection.h>

#define KEECHAIN_DEBUG_ALLOWED // Закомментировать в продакшене

static constexpr auto PROTOCOL_MAGIC_BEGIN = "WARLIN";

static constexpr auto PROTOCOL_DEBUG_BEGIN = "DEVICE_DEBUG";

static constexpr auto PROTOCOL_ERROR_BEGIN = "WARLIN_ERROR";

static constexpr auto DEFAULT_DELIMITER = "<PART>";

static constexpr auto DEFAULT_BAUDRATE = 115600;

Z_ENUM_NS(
    PROTOCOL_REQUEST_TYPE,
    DISCOVER,
    SYNC,
    DH_KEY_PROPOSAL,
    STORE_SECRET,
    GET_TOTP
);

Z_ENUM_NS(
    PROTOCOL_RESPONSE_TYPE,
    ACK,
    SYNC
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
    private:
        std::unordered_map<PROTOCOL_REQUEST_TYPE, void(*)(std::deque<std::string>&)> listeners{};
};

std::string NameOf(PROTOCOL_REQUEST_TYPE);

std::string NameOf(PROTOCOL_RESPONSE_TYPE);

void SendDebugMessage(const char * message);

void SendDebugMessage(const char * part1, const char * part2);

void SendErrorMessage(const char * message);

void SendErrorMessage(const char * part1, const char * part2);

#endif // Guard
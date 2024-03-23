#pragma once

#include <Arduino.h>
#include <deque>
#include <unordered_map>

#define KEECHAIN_DEBUG_ALLOWED // Закомментировать в продакшене

static constexpr auto PROTOCOL_MAGIC_BEGIN = "WARLIN";

static constexpr auto PROTOCOL_DEBUG_BEGIN = "PROTOCOL_DEBUG";

static constexpr auto DEFAULT_DELIMITER = "<PART>";

static constexpr auto DEFAULT_BAUDRATE = 115600;

enum class PROTOCOL_REQUEST_TYPE
{
    DISCOVER,
    DH_KEY_PROPOSAL,
    CAPABILITY_QUERY,
    STORE_SECRET,
    GET_TOTP
};

class Warlin_
{
    public:
        void bind(PROTOCOL_REQUEST_TYPE, void(*)(std::deque<std::string>&));
        bool available();
        void process();
        Warlin_();
        void writeString(const std::string & str);
        void writeStrings(const std::initializer_list<std::string> & args);
    private:
        std::unordered_map<PROTOCOL_REQUEST_TYPE, void(*)(std::deque<std::string>&)> listeners{};
};

void SendDebugMessage(const char * message);

void SendDebugMessage(const char * part1, const char * part2);

std::string ProtocolRequestTypeToString(const PROTOCOL_REQUEST_TYPE & type);

std::pair<bool, PROTOCOL_REQUEST_TYPE> StringToProtocolRequestType(const std::string & str);
#include "itch_parser.hpp"

#include <iomanip>
#include <iostream>
#include <vector>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include <zlib.h>

void ItchParser::Parse(const std::string& filename)
{
    constexpr auto messageLengths = [] {
        std::array<size_t, std::numeric_limits<uint8_t>::max()> lengths{};
        lengths['S'] = 12;
        lengths['R'] = 39;
        lengths['H'] = 25;
        lengths['Y'] = 20;
        lengths['L'] = 26;
        lengths['V'] = 35;
        lengths['W'] = 12;
        lengths['K'] = 28;
        lengths['J'] = 35;
        lengths['h'] = 1;
        lengths['A'] = 36;
        lengths['F'] = 40;
        lengths['E'] = 31;
        lengths['C'] = 36;
        lengths['X'] = 23;
        lengths['D'] = 19;
        lengths['U'] = 35;
        lengths['P'] = 44;
        lengths['Q'] = 40;
        lengths['B'] = 19;
        lengths['I'] = 50;
        lengths['O'] = 1;
        return lengths;
    }();

    gzFile gfile = gzopen(filename.c_str(), "rb");
    if (!gfile)
    {
        std::cerr << "Failed to open file: " << filename << '\n';
        return;
    }

    while (true)
    {
        const size_t messageLength = (gzgetc(gfile) << 8) | gzgetc(gfile);
        const auto messageType = static_cast<char>(gzgetc(gfile));

        if (messageLengths[messageType] != messageLength)
        {
            if (gzeof(gfile)) { break; }
            std::cerr << "Failed to parse data. Char: " << messageType << " Length: " << messageLength;
            return;
        }

        std::array<char, 50> message{};
        const size_t bytesRead = gzread(gfile, message.data(), static_cast<uint32_t>(messageLength - 1));

        if (bytesRead + 1 != messageLength)
        {
            if (gzeof(gfile)) { break; }
            std::cerr << "Failed to read data. Char: " << messageType << " Bytes read: " << bytesRead;
            return;
        }

        switch (messageType)
        {
        case 'P': ProcessTradeMessage(message); break;
        default: break; // Ignore other message types for this exercise.
        }
    }

    gzclose(gfile);
}

uint32_t ItchParser::NetworkToHost(const uint32_t val)
{
#ifdef _WIN32
    return _byteswap_ulong(val);
#else
    return ntohl(val);
#endif
}

uint64_t ItchParser::NetworkToHost(const uint64_t val)
{
#ifdef _WIN32
    return _byteswap_uint64(val);
#else
    return be64toh(val);
#endif
}

void ItchParser::ProcessTradeMessage(const std::array<char, 50>& message)
{
    constexpr uint64_t nsPerHour = 60ULL * 60ULL * 1E9;

    const uint64_t timestamp = NetworkToHost(*reinterpret_cast<const uint64_t*>(message.data() + 4)) & ((1ULL << 48) - 1);
    const uint32_t shares = NetworkToHost(*reinterpret_cast<const uint32_t*>(message.data() + 19));
    const double price = NetworkToHost(*reinterpret_cast<const uint32_t*>(message.data() + 31)) / 10000.0;
    const std::string ticker(message.data() + 23, 8);

    const uint32_t hour = (timestamp / nsPerHour) % 24;
    TradeData& tradeData = stockData_[ticker][hour];
    tradeData.volume += shares;
    tradeData.priceVolume += shares * price;
}

void ItchParser::CalculateAndPrintVwap() const
{
    for (const auto& [ticker, hourlyData] : stockData_)
    {
        for (uint32_t hour = 0; hour < 24; ++hour)
        {
            std::cout << std::left << std::setw(10) << "Ticker:" << std::setw(10) << ticker << std::setw(10) << "Hour:" << std::setw(10) << hour
                      << std::setw(10) << "VWAP:" << std::fixed << std::setprecision(4) << hourlyData[hour].GetVwap() << '\n';
        }
    }
}

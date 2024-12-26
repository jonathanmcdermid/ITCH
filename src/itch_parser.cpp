#include "itch_parser.hpp"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include <zlib.h>

void ItchParser::Parse(const std::string& filename)
{
    // Increasing the buffer size will reduce the number of reads from the file and improve performance, with diminishing returns.
    constexpr size_t maxBufferSize = 1 << 19; // 512 KB

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

    constexpr auto maxMessageLength = [&] {
        size_t max = 0;
        for (const auto& length : messageLengths)
        {
            if (length > max) { max = length; }
        }
        return max;
    }();

    gzFile gfile = gzopen(filename.c_str(), "rb");
    if (!gfile)
    {
        std::cerr << "Failed to open file: " << filename << '\n';
        return;
    }

    size_t bufferOffset = 0;
    size_t bufferSize = 0;

    std::array<char, maxBufferSize> buffer{};

    while (true)
    {
        // Read more data if the buffer is empty.
        if (bufferOffset == bufferSize)
        {
            int res = gzread(gfile, buffer.data(), maxBufferSize);

            if (res == 0) { break; }
            if (res == -1)
            {
                std::cerr << "Failed to read from file.\n";
                gzclose(gfile);
                return;
            }

            bufferOffset = 0;
            bufferSize = static_cast<size_t>(res);
        }

        // If the message might be truncated, move the remaining data to the beginning of the buffer and read more data.
        if (bufferSize - bufferOffset < 2 + maxMessageLength) // 2 bytes for message length.
        {
            std::memmove(buffer.data(), &buffer[bufferOffset], bufferSize - bufferOffset);
            const size_t leftoverSize = bufferSize - bufferOffset;
            int res = gzread(gfile, &buffer[leftoverSize], static_cast<uint32_t>(maxBufferSize - leftoverSize));

            if (res == 0) { break; }
            if (res == -1)
            {
                std::cerr << "Failed to read from file.\n";
                gzclose(gfile);
                return;
            }

            bufferOffset = 0;
            bufferSize = leftoverSize + static_cast<size_t>(res);
        }

        const size_t messageLength = (buffer[bufferOffset] << 8) | buffer[bufferOffset + 1];
        const char messageType = buffer[bufferOffset + 2];
        bufferOffset += 3;

        // Check if the character represents a valid message type.
        if (messageLengths[messageType] == 0)
        {
            std::cerr << "Unknown message type: " << messageType << '\n';
            gzclose(gfile);
            return;
        }

        // Check if the message length is correct for the message type.
        if (messageLengths[messageType] != messageLength)
        {
            std::cerr << "Invalid message. Type: " << messageType << " Length: " << messageLength << '\n';
            gzclose(gfile);
            return;
        }

        switch (messageType)
        {
        case 'P': ProcessTradeMessage(&buffer[bufferOffset]); break;
        default: break; // Ignore other message types for this exercise.
        }

        bufferOffset += messageLength - 1;
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

void ItchParser::ProcessTradeMessage(const char* message)
{
    constexpr uint64_t nsPerHour = 60ULL * 60ULL * 1E9;

    const uint64_t timestamp = NetworkToHost(*reinterpret_cast<const uint64_t*>(message + 4)) & ((1ULL << 48) - 1);
    const uint32_t shares = NetworkToHost(*reinterpret_cast<const uint32_t*>(message + 19));
    const double price = NetworkToHost(*reinterpret_cast<const uint32_t*>(message + 31)) / 10000.0;
    const std::string ticker(message + 23, 8);

    const uint32_t hour = (timestamp / nsPerHour) % 24;
    TradeData& tradeData = stockData[ticker][hour];
    tradeData.volume += shares;
    tradeData.priceVolume += shares * price;
}

void ItchParser::CalculateAndPrintVwap() const
{
    for (const auto& [ticker, hourlyData] : stockData)
    {
        for (uint32_t hour = 0; hour < 24; ++hour)
        {
            std::cout << std::left << std::setw(10) << "Ticker:" << std::setw(10) << ticker << std::setw(10) << "Hour:" << std::setw(10) << hour
                      << std::setw(10) << "VWAP:" << std::fixed << std::setprecision(4) << hourlyData[hour].GetVwap() << '\n';
        }
    }
}

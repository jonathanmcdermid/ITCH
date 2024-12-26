#ifndef ITCH_PARSER_HPP
#define ITCH_PARSER_HPP

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>

class ItchParser
{
  public:
    explicit ItchParser() = default;

    void Parse(const std::string& filename);

    void CalculateAndPrintVwap() const;

    void Reset() { stockData.clear(); }

  private:
    struct TradeData
    {
        uint64_t volume{};
        double priceVolume{};
        [[nodiscard]] double GetVwap() const { return volume == 0 ? std::numeric_limits<double>::quiet_NaN() : priceVolume / volume; }
    };

    std::unordered_map<std::string, std::array<TradeData, 24>> stockData;

    [[nodiscard]] static uint32_t NetworkToHost(uint32_t val);

    [[nodiscard]] static uint64_t NetworkToHost(uint64_t val);

    void ProcessTradeMessage(const char* message);
};

#endif // ITCH_PARSER_HPP

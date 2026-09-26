#include <immintrin.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

template <typename To>
inline static constexpr auto cast =
    []<typename From> [[nodiscard, gnu::always_inline]] (From&& v) noexcept
{
    return static_cast<To>(std::forward<From>(v));
};

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// NOLINTBEGIN
static constexpr u64 BIAS = 0x6060606060606060ull, EXT = 0x1F1F1F1F1F1F1F1Full;
[[gnu::hot, gnu::target("bmi2")]]
inline u64 packStr(const std::string& s) noexcept
{
    const char* c8 = s.c_str();
    u8 n8 = s.length() & 0xFF;
    u8 sh = 50;
    u64 r = 0;
    if (n8 >= 8)
    {
        const u64 qw = *reinterpret_cast<const u64*>(c8) - BIAS;
        sh -= 40;
        r += _pext_u64(__builtin_bswap64(qw), EXT) << sh;
        c8 += 8;
        n8 -= 8;
    }
    if (n8 >= 4)
    {
        const u32 dw = *reinterpret_cast<const u32*>(c8) - (BIAS & 0xFFFFFFFFu);
        sh -= 20;
        r += u64{_pext_u32(__builtin_bswap32(dw), EXT & 0xFFFFFFFFu)} << sh;
        c8 += 4;
        n8 -= 4;
    }
    if (n8 >= 2)
    {
        const u16 w = *reinterpret_cast<const u16*>(c8) - (BIAS & 0xFFFFu);
        sh -= 10;
        r += u64{_pext_u32(__builtin_bswap32(w) >> 16, EXT & 0xFFFFu)} << sh;
        c8 += 2;
        n8 -= 2;
    }
    if (n8)
    {
        sh -= 5;
        r += (cast<u64>(*c8) - (BIAS & 0xFFu)) << sh;
        c8 += 1;
        n8 -= 1;
    }
    return r;
}

[[gnu::hot, gnu::target("bmi2")]]
inline std::string unpackStr(const u64 v) noexcept
{
    std::string r(10, '\0');
    char* c8 = r.data();
    *reinterpret_cast<u64*>(c8) =
        __builtin_bswap64(_pdep_u64(v >> 10, EXT)) + BIAS;
    *reinterpret_cast<u16*>(c8 + 8u) =
        __builtin_bswap32(
            cast<u32>(_pdep_u32(cast<u32>(v), EXT & 0xFFFFu) + (BIAS & 0xFFFFu))
            << 16) &
        0xFFFF;
    const auto end = r.find(BIAS & 0xFFu);
    if (end != std::string::npos) r.resize(end);
    return r;
}
// NOLINTEND
#ifndef LC_LOCAL_BUILD
auto init = []()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    return 'c';
}();
#endif

class Solution
{
public:
    struct Entry
    {
        u64 key;
        u64 value;
    };

    inline static Entry entries[1 << 17];

    template <u32 capacity>
    struct HashTable
    {
        [[gnu::always_inline, gnu::no_sanitize_address]] Entry& slot(
            u64 key) noexcept
        {
            u64 z = key + 0x9e3779b97f4a7c15ULL;
            z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
            u32 i = cast<u32>(z >> 32U) & (capacity - 1);
            while (entries[i].key && entries[i].key != key)
            {
                i = (i + 1) & (capacity - 1);
            }
            return entries[i];
        }
    };

    [[gnu::no_sanitize_address]] static std::string_view
    consume(std::string_view s, size_t& i, char until) noexcept
    {
        size_t begin = i;
        i = s.find(until, i);
        return s.substr(begin, i - begin);
    }

    [[gnu::no_sanitize_address]] static std::string decode(
        auto& m,
        std::string_view key) noexcept
    {
        const auto& entry = m.slot(packStr(std::string{key}));
        if (entry.key)
        {
            return unpackStr(entry.value);
        }
        return "?";
    }

    template <u32 capacity>
    [[gnu::no_sanitize_address]] static std::string impl(
        std::string_view s,
        const std::vector<std::vector<std::string>>& knowledge) noexcept
    {
        std::string r;
        r.reserve((s.size() * 3) / 2);
        HashTable<capacity> m;
        std::memset(entries, 0, capacity * sizeof(Entry));
        for (auto& t : knowledge)
        {
            auto key = packStr(t[0]);
            m.slot(key) = {key, packStr(t[1])};
        }

        size_t i = 0, n = s.size();

        while (i < n)
        {
            r += consume(s, i, '(');
            if (i < n)
            {
                ++i;  // '('
                r += decode(m, consume(s, i, ')'));
                ++i;  // ')'
            }
        }

        return r;
    }

    [[gnu::no_sanitize_address]] std::string evaluate(
        std::string_view s,
        const std::vector<std::vector<std::string>>& knowledge) const noexcept
    {
        constexpr std::array fns{
            impl<1 << 0>,
            impl<1 << 1>,
            impl<1 << 2>,
            impl<1 << 3>,
            impl<1 << 4>,
            impl<1 << 5>,
            impl<1 << 6>,
            impl<1 << 7>,
            impl<1 << 8>,
            impl<1 << 9>,
            impl<1 << 10>,
            impl<1 << 11>,
            impl<1 << 12>,
            impl<1 << 13>,
            impl<1 << 14>,
            impl<1 << 15>,
            impl<1 << 16>,
            impl<1 << 17>,
        };
        const auto index = std::min(
            17,
            std::bit_width(knowledge.size() + knowledge.size() / 2));
        return fns[cast<size_t>(index)](s, knowledge);
    }
};
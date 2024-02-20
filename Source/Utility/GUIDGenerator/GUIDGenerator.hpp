#ifndef SHIFT_GUIDGENERATOR_HPP
#define SHIFT_GUIDGENERATOR_HPP

#include <unordered_map>
#include <random>

namespace sft {
    //! Shift GUID
    class SGUID {
    public:
        SGUID(uint64_t id): m_id{id} {}

        operator uint64_t() const { return m_id; }
    private:
        uint64_t m_id;
    };

    class GUIDGenerator {
    public:
        static GUIDGenerator& GetInstance() {
            static GUIDGenerator tm;
            return tm;
        }

        [[nodiscard]] SGUID Guid() const;

        GUIDGenerator(const GUIDGenerator& t) = delete;
        GUIDGenerator& operator=(const GUIDGenerator& t) = delete;
    private:
        GUIDGenerator() {};
    };
}

namespace std {
    template<>
    struct hash<sft::SGUID>
    {
        std::size_t operator()(const sft::SGUID& uuid) const {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };
}

#endif //SHIFT_GUIDGENERATOR_HPP

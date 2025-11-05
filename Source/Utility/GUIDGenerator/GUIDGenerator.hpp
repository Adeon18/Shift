#ifndef SHIFT_GUIDGENERATOR_HPP
#define SHIFT_GUIDGENERATOR_HPP

#include <unordered_map>
#include <random>

namespace Shift {
    //! Shift GUID
    class SGUID {
    public:
        SGUID() = default;
        SGUID(uint64_t id): m_id{id} {}
        SGUID(const SGUID& guid) = default;
        SGUID& operator=(const SGUID& guid) = default;

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
    struct hash<Shift::SGUID>
    {
        std::size_t operator()(const Shift::SGUID& uuid) const {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };
}

#endif //SHIFT_GUIDGENERATOR_HPP

#pragma once

#include <random>

static std::random_device                      s_random_device;
static std::mt19937_64                         s_randome_engine(s_random_device());
static std::uniform_int_distribution<uint64_t> s_uniform_int_distribution;

class ID
{
  public:
    ID() : m_uuid(s_uniform_int_distribution(s_randome_engine)){};

    ID(const uint64_t& uuid) : m_uuid(uuid)
    {
    }
    uint64_t get_uuid() const
    {
        return m_uuid;
    };
    const ID& getID() const
    {
        return *this;
    };

  private:
    uint64_t m_uuid;
};

// make hashable

namespace std
{
template <> struct hash<ID>
{
    std::size_t operator()(const ID& uuid) const
    {
        return hash<uint64_t>()(uuid.get_uuid());
    }
};
} // namespace std

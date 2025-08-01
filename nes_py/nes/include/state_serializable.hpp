#pragma once
#include "state.hpp"

namespace NES {
class IStateSerializable {
public:
    virtual ~IStateSerializable() = default;

    /** Return the four‑byte ASCII chunk identifier. */
    virtual std::string chunk_id() const = 0;

    /** Write the object’s state into the current chunk body. */
    virtual void save_state(StateWriter&) const = 0;

    /** Read the object’s state from the current chunk body. */
    virtual void load_state(StateReader&)       = 0;
};

}

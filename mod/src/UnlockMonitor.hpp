#pragma once

#include <cstdint>
#include <optional>

namespace iconstat {
class UnlockMonitorState final {
public:
    void tick(float dt) {
        if (m_retrySeconds > 0.f) m_retrySeconds -= dt;
    }

    void reset() {
        m_baseline.reset();
        m_requestInFlight = false;
        m_retrySeconds = 0.f;
    }

    bool observe(uint64_t fingerprint, bool apiBusy) {
        if (!m_baseline) {
            m_baseline = fingerprint;
            return false;
        }
        if (*m_baseline == fingerprint || m_requestInFlight || apiBusy || m_retrySeconds > 0.f) return false;
        m_requestInFlight = true;
        return true;
    }

    void complete(bool success, uint64_t attemptedFingerprint) {
        m_requestInFlight = false;
        if (success) {
            m_baseline = attemptedFingerprint;
            m_retrySeconds = 0.f;
        } else {
            m_retrySeconds = 30.f;
        }
    }

    bool requestInFlight() const { return m_requestInFlight; }
    float retrySeconds() const { return m_retrySeconds; }

private:
    std::optional<uint64_t> m_baseline;
    bool m_requestInFlight = false;
    float m_retrySeconds = 0.f;
};
} // namespace iconstat

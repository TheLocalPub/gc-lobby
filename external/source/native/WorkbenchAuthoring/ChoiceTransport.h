#pragma once
#include <QString>

namespace gc {
// Fixed startup task; caller must have validated the exact host and Qt hashes.
// Uses the current editor's port and enables only its NET API setting.
QString initializeChoiceTransport(quint16 &port);
}

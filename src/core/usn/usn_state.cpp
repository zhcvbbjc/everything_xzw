// usn_state.cpp
#include "core/usn/usn_state.h"

UsnState load_usn_state() {
    return UsnState{}; // 无状态启动
}

void save_usn_state(const UsnState&) {
    // TODO: persist to disk
}

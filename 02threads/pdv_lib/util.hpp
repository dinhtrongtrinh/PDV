#pragma once

/** Magic macro that runs the passed block as a static initializer, before `main` is entered. */
#define PDV_STATIC_INIT(NAME, BLOCK) namespace pdv::_::static_init { \
    [[maybe_unused]] inline int NAME{([] { \
        BLOCK; \
        return 0; \
    })()}; \
}

// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

/*
 * Minimal RAII wrappers for key Axoverlay types.
 */
#pragma once

#include <axoverlay2.h>
#include <memory>

struct AxoErr {
    axo_err* err = nullptr;

    AxoErr() = default;

    ~AxoErr() { axo_err_clear(&err); }

    AxoErr(const AxoErr&)            = delete;
    AxoErr& operator=(const AxoErr&) = delete;

    AxoErr(AxoErr&& o) { std::swap(this->err, o.err); }

    AxoErr& operator=(AxoErr&& o) {
        std::swap(this->err, o.err);
        return *this;
    }
};

struct AxoProps_deleter {
    void operator()(axo_props* p) { axo_props_free(p); }
};
using AxoProps = std::unique_ptr<axo_props, AxoProps_deleter>;

struct AxoMatch_deleter {
    void operator()(axo_match* p) { axo_match_free(p); }
};
using AxoMatch = std::unique_ptr<axo_match, AxoMatch_deleter>;

struct AxoDetailedFormat_deleter {
    void operator()(axo_detailed_format* p) { axo_detailed_format_free(p); }
};
using AxoDetailedFormat = std::unique_ptr<axo_detailed_format, AxoDetailedFormat_deleter>;

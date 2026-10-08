/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "translation.h"
#include "types/ret.h"

namespace au::podcast {
static constexpr int PODCAST_FIRST = 6500; // TODO This has to go in framework's ret.h

enum class Err {
    Undefined       = int(muse::Ret::Code::Undefined),
    NoError         = int(muse::Ret::Code::Ok),
    Cancel          = int(muse::Ret::Code::Cancel),
    UnknownError    = PODCAST_FIRST,

    NoProjectOpened,
    NoAudioTracks,
    EmptyRange,
    NoSpeechDetected,
    FailedToApplyEdits,
    FailedToCreateLabelTrack
};

inline muse::Ret make_ret(Err e)
{
    const int retCode = static_cast<int>(e);

    switch (e) {
    case Err::Undefined: return muse::Ret(retCode);
    case Err::NoError: return muse::Ret(retCode);
    case Err::Cancel: return muse::Ret(retCode);
    case Err::UnknownError: return muse::Ret(retCode);
    case Err::NoProjectOpened: return muse::Ret(retCode, muse::trc("podcast", "No project is opened"));
    case Err::NoAudioTracks: return muse::Ret(retCode, muse::trc("podcast", "There are no audio tracks to process"));
    case Err::EmptyRange: return muse::Ret(retCode, muse::trc("podcast", "The range to process is empty"));
    case Err::NoSpeechDetected: return muse::Ret(retCode, muse::trc("podcast", "No speech was detected in the processed range"));
    case Err::FailedToApplyEdits: return muse::Ret(retCode, muse::trc("podcast", "The changes could not be applied"));
    case Err::FailedToCreateLabelTrack: return muse::Ret(retCode, muse::trc("podcast", "The label track could not be created"));
    }

    return muse::Ret(retCode);
}
}

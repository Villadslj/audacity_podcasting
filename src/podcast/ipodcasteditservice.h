/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "global/modularity/imoduleinterface.h"
#include "global/types/ret.h"

#include "podcasttypes.h"

namespace au::podcast {
//! NOTE Applies the results of the speech detection to the project.
//! Every operation produces exactly one undo step.
class IPodcastEditService : MODULE_EXPORT_INTERFACE
{
    INTERFACE_ID(au::podcast::IPodcastEditService)

public:
    virtual ~IPodcastEditService() = default;

    //! Removes the non-speech parts of every track without moving anything,
    //! so that the tracks stay in sync.
    virtual muse::Ret stripSilence() = 0;

    //! Shortens the silences that all processed tracks share, moving
    //! everything that follows a gap to the left on every track.
    virtual muse::Ret tightenGaps() = 0;

    //! Adds a label track holding one label per detected speech range,
    //! so that the detection can be checked before anything is applied.
    virtual muse::Ret markSpeech() = 0;
};
}

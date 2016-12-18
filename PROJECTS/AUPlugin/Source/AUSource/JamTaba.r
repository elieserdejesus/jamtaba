#include <AudioUnit/AudioUnit.r>

#include "JamTabaVersion.h"

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define kAudioUnitResID_JamTaba				1000

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Filter~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define RES_ID			kAudioUnitResID_JamTaba
#define COMP_TYPE		kAudioUnitType_Effect
#define COMP_SUBTYPE	JAMTABA_COMP_SUBTYPE
#define COMP_MANUF		JAMTABA_COMP_MANF

#define VERSION			kJamTabaVersion
#define NAME			"Jamtaba : Ninjam"
#define DESCRIPTION		"Jamtaba - Ninjam Client"
#define ENTRY_POINT		"FilterEntry"

#include "AUResources.r"
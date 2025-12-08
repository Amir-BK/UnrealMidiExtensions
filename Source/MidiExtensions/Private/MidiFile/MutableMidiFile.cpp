// Copyright notice in the Description page of Project Settings.

#include "MidiFile/MutableMidiFile.h"
#include "CoreMinimal.h"
#include "HarmonixMidi/MidiTrack.h"
#include "HarmonixMidi/MidiEvent.h"
#include "HarmonixMidi/MidiMsg.h"


TSharedPtr<Audio::IProxyData> UMutableMidiFile::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
    LinkedMidiData = FMidiNotesData::BuildFromMidiFile(this);
	
	return UMidiFile::CreateProxyData(InitParams);
}

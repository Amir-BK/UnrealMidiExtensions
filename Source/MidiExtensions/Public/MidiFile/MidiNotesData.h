// Copyright Amir Ben-Kiki 2025

#pragma once

#include "CoreMinimal.h"
#include "MidiNotesData.generated.h"

// This struct links note-on and note-off events
USTRUCT(BlueprintType)
struct FLinkedMidiNote
{
    GENERATED_BODY()

    UPROPERTY()
    int32 NoteOnTick = 0;

    UPROPERTY()
    int32 NoteOffTick = 0;

    UPROPERTY()
    int8 Velocity = 0;

    UPROPERTY()
    int8 NoteNumber = 0;


};

USTRUCT(BlueprintType)
struct FMidiNotesTrack
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FLinkedMidiNote> Notes;

    UPROPERTY()
    FString TrackName;

    UPROPERTY()
    int32 TrackIndex = INDEX_NONE;

    /** MIDI channel index (0-15) - separate from track as one track can have multiple channels */
    UPROPERTY()
    int32 ChannelIndex = INDEX_NONE;


};



USTRUCT(BlueprintType)
struct MIDIEXTENSIONS_API FMidiNotesData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMidiNotesTrack> Tracks;

	static TSharedPtr<FMidiNotesData> BuildFromMidiFile(class UMidiFile* MidiFile);
};

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/MidiExtensions.MidiExtensionsHelperLib.MakeMidiFileIterator"))
struct MIDIEXTENSIONS_API FMidiFileIterator
{
    GENERATED_BODY()
    UPROPERTY()
    FMidiNotesData MidiData;
    UPROPERTY()
    int32 CurrentTrackIndex = 0;
    UPROPERTY()
    int32 CurrentNoteIndex = 0;
    bool IsValid() const
    {
        if (CurrentTrackIndex < 0 || CurrentTrackIndex >= MidiData.Tracks.Num())
        {
            return false;
        }
        if (CurrentNoteIndex < 0 || CurrentNoteIndex >= MidiData.Tracks[CurrentTrackIndex].Notes.Num())
        {
            return false;
        }
        return true;
    }
    FLinkedMidiNote GetCurrentNote() const
    {
        const FMidiNotesTrack& Track = MidiData.Tracks[CurrentTrackIndex];
        check(CurrentNoteIndex >= 0 && CurrentNoteIndex < Track.Notes.Num());
        return Track.Notes[CurrentNoteIndex];
    }
};

    //FLinkedMidiNote GetCurrentNote() const
    //{
    //   // const FMidiNotesTrack& Track = MidiData.Tracks[CurrentTrackIndex];
    //   // check(CurrentNoteIndex >= 0 && CurrentNoteIndex < Track.Notes.Num());
    //    return Track.Notes[CurrentNoteIndex];
    //}

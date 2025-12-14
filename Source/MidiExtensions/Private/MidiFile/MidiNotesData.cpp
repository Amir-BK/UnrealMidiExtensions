// Copyright Amir Ben-Kiki 2025

#include "MidiFile/MidiNotesData.h"
#include "HarmonixMidi/MidiFile.h"

// Key for tracking active notes: combines note number and channel
struct FNoteChannelKey
{
    int32 NoteNumber;
    int32 Channel;
    
    bool operator==(const FNoteChannelKey& Other) const
    {
        return NoteNumber == Other.NoteNumber && Channel == Other.Channel;
    }
    
    friend uint32 GetTypeHash(const FNoteChannelKey& Key)
    {
        return HashCombine(GetTypeHash(Key.NoteNumber), GetTypeHash(Key.Channel));
    }
};

TSharedPtr<FMidiNotesData> FMidiNotesData::BuildFromMidiFile(UMidiFile* MidiFile)
{
    // Create container
    TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> LinkedMidiData = MakeShared<FMidiNotesData, ESPMode::ThreadSafe>();

    if (!MidiFile)
    {
        return LinkedMidiData;
    }

    // First pass: discover all unique (Track, Channel) combinations
    TArray<TTuple<int32, int32>> FoundChannels;
    
    const int32 NumTracks = MidiFile->GetNumTracks();
    for (int32 TrackIdx = 0; TrackIdx < NumTracks; ++TrackIdx)
    {
        const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
        if (!Track) continue;
        
        for (const FMidiEvent& Event : Track->GetEvents())
        {
            if (Event.GetMsg().IsNoteOn())
            {
                const int32 Channel = Event.GetMsg().GetStdChannel();
                FoundChannels.AddUnique(TTuple<int32, int32>(TrackIdx, Channel));
            }
        }
    }
    
    // If no notes found, fall back to one entry per track
    if (FoundChannels.IsEmpty())
    {
        for (int32 TrackIdx = 0; TrackIdx < NumTracks; ++TrackIdx)
        {
            const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
            if (Track)
            {
                FoundChannels.Add(TTuple<int32, int32>(TrackIdx, Track->GetPrimaryMidiChannel()));
            }
        }
    }
    
    // Create output tracks for each (Track, Channel) combination
    LinkedMidiData->Tracks.SetNum(FoundChannels.Num());
    for (int32 Idx = 0; Idx < FoundChannels.Num(); ++Idx)
    {
        const int32 TrackIdx = FoundChannels[Idx].Get<0>();
        const int32 ChannelIdx = FoundChannels[Idx].Get<1>();
        
        const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
        
        FMidiNotesTrack& OutTrack = LinkedMidiData->Tracks[Idx];
        const FString* NamePtr = Track ? Track->GetName() : nullptr;
        
        // Create descriptive name including channel if not primary
        const bool bIsPrimaryChannel = Track && (Track->GetPrimaryMidiChannel() == ChannelIdx);
        if (bIsPrimaryChannel && NamePtr)
        {
            OutTrack.TrackName = *NamePtr;
        }
        else
        {
            OutTrack.TrackName = FString::Printf(TEXT("%s Ch:%d"), 
                NamePtr ? **NamePtr : TEXT("Track"), ChannelIdx);
        }
        
        OutTrack.TrackIndex = TrackIdx;
        OutTrack.ChannelIndex = ChannelIdx;
    }
    
    // Second pass: link notes for each track, filtering by channel
    for (int32 TrackIdx = 0; TrackIdx < NumTracks; ++TrackIdx)
    {
        const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
        if (!Track) continue;
        
        // Map of active notes -> pair<OnTick, Velocity> using composite key
        TMap<FNoteChannelKey, TPair<int32, int32>> ActiveNotes;
        
        for (const FMidiEvent& Event : Track->GetEvents())
        {
            const FMidiMsg& Msg = Event.GetMsg();
            
            if (Msg.IsNoteOn())
            {
                const int32 Note = (int32)Msg.GetStdData1();
                const int32 Velocity = (int32)Msg.GetStdData2();
                const int32 Channel = Msg.GetStdChannel();
                
                FNoteChannelKey Key{ Note, Channel };
                ActiveNotes.Add(Key, TPair<int32, int32>(Event.GetTick(), Velocity));
            }
            else if (Msg.IsNoteOff())
            {
                const int32 Note = (int32)Msg.GetStdData1();
                const int32 Channel = Msg.GetStdChannel();
                
                FNoteChannelKey Key{ Note, Channel };
                TPair<int32, int32>* OnData = ActiveNotes.Find(Key);
                if (OnData)
                {
                    // Find the output track for this (TrackIdx, Channel) combination
                    const int32 OutputTrackIdx = FoundChannels.IndexOfByPredicate(
                        [TrackIdx, Channel](const TTuple<int32, int32>& Entry)
                        {
                            return Entry.Get<0>() == TrackIdx && Entry.Get<1>() == Channel;
                        });
                    
                    if (OutputTrackIdx != INDEX_NONE)
                    {
                        FLinkedMidiNote Linked;
                        Linked.NoteOnTick = OnData->Key;
                        Linked.NoteOffTick = Event.GetTick();
                        Linked.Velocity = (int8)OnData->Value;
                        Linked.NoteNumber = (int8)Note;
                        LinkedMidiData->Tracks[OutputTrackIdx].Notes.Add(Linked);
                    }
                    
                    ActiveNotes.Remove(Key);
                }
            }
        }
    }

	// Finally sort notes in each track by NoteOnTick
    for (FMidiNotesTrack& NotesTrack : LinkedMidiData->Tracks)
    {
        NotesTrack.Notes.Sort([](const FLinkedMidiNote& A, const FLinkedMidiNote& B)
        {
            return A.NoteOnTick < B.NoteOnTick;
        });
	} 
    
    return LinkedMidiData;
}

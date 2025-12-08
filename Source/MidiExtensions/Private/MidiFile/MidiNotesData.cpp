// Copyright Amir Ben-Kiki 2025

#include "MidiFile/MidiNotesData.h"
#include "HarmonixMidi/MidiFile.h"

// This translation unit ensures UHT/UObject codegen sees the USTRUCT types
// defined in MidiNotesData.h within the module. No additional implementation
// is required here.

TSharedPtr<FMidiNotesData> FMidiNotesData::BuildFromMidiFile(UMidiFile* MidiFile)
{
    // Create container
    TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> LinkedMidiData = MakeShared<FMidiNotesData, ESPMode::ThreadSafe>();

    // Iterate all MIDI tracks
    const int32 NumTracks = MidiFile->GetNumTracks();
    LinkedMidiData->Tracks.Reserve(NumTracks);

    for (int32 TrackIdx = 0; TrackIdx < NumTracks; ++TrackIdx)
    {
        const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
        if (!Track)
        {
            continue;
        }

        FMidiNotesTrack OutTrack;
        const FString* NamePtr = Track->GetName();
        OutTrack.TrackName = NamePtr ? *NamePtr : FString();
        OutTrack.TrackIndex = TrackIdx;
        OutTrack.Notes.Empty();

        // Map of active notes -> pair<OnTick, Velocity>
        TMap<int32, TPair<int32, int32>> ActiveNotes; // key: NoteNumber, value: (OnTick, Velocity)

        const FMidiEventList& Events = Track->GetUnsortedEvents();
        for (const FMidiEvent& Event : Events)
        {
            const FMidiMsg& Msg = Event.GetMsg();

            if (Msg.IsNoteOn())
            {
                const int32 Note = (int32)Msg.GetStdData1();
                const int32 Velocity = (int32)Msg.GetStdData2();
                ActiveNotes.Add(Note, TPair<int32, int32>(Event.GetTick(), Velocity));
            }
            else if (Msg.IsNoteOff())
            {
                const int32 Note = (int32)Msg.GetStdData1();
                TPair<int32, int32>* OnData = ActiveNotes.Find(Note);
                if (OnData)
                {
                    FLinkedMidiNote Linked;
                    Linked.NoteOnTick = OnData->Key;
                    Linked.NoteOffTick = Event.GetTick();
                    Linked.Velocity = (int8)OnData->Value;
                    Linked.NoteNumber = (int8)Note;
                    OutTrack.Notes.Add(Linked);
                    ActiveNotes.Remove(Note);
                }
                // If no matching NoteOn, ignore
            }
        }

        // Optionally, flush any lingering active notes by creating notes with NoteOffTick equal to last event tick
        // for (const auto& Pair : ActiveNotes) { /* left open intentionally */ }

        LinkedMidiData->Tracks.Add(MoveTemp(OutTrack));
    }
    
    return LinkedMidiData;
}

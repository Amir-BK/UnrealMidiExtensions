// Copyright notice in the Description page of Project Settings.

#include "MidiPianoroll.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"

TSharedRef<SWidget> UMidiPianoroll::RebuildWidget()
{
    TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> MidiData;
    TSharedPtr<FSongMaps, ESPMode::ThreadSafe> SongsMap;

    if(LinkedMidiFile)
    {
        MidiData = FMidiNotesData::BuildFromMidiFile(LinkedMidiFile);
        SongsMap = MakeShared<FSongMaps, ESPMode::ThreadSafe>(*LinkedMidiFile->GetSongMaps());
        VisualizationData = FMidiFileVisualizationData::BuildFromMidiFile(LinkedMidiFile);
    }

    SAssignNew(PianorollWidget, SMidiPianoroll)
        .LinkedMidiData(MidiData)
        .LinkedSongsMap(SongsMap)
        // Bind to getter using lambda - will be called each frame during paint
        .VisualizationData(TAttribute<FMidiFileVisualizationData>::CreateLambda([this]() { return GetVisualizationData(); }))
        .PianorollStyle(&FMidiPianorollStyle::GetDefault())
        .Offset(FVector2D::ZeroVector)
        .Zoom(FVector2D(1.0f, 1.0f));

    return PianorollWidget.ToSharedRef();
}

void UMidiPianoroll::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    PianorollWidget.Reset();
}

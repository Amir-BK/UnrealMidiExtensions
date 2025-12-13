// Copyright notice in the Description page of Project Settings.

#include "MidiPianoroll.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"


void UMidiPianoroll::SetMidiFile(UMidiFile* InMidiFile)
{
    LinkedMidiFile = InMidiFile;
    if (PianorollWidget.IsValid())
    {
        if (LinkedMidiFile)
        {
            TSharedPtr<FSongMaps, ESPMode::ThreadSafe> SongsMap = MakeShared<FSongMaps, ESPMode::ThreadSafe>(*LinkedMidiFile->GetSongMaps());
            PianorollWidget->SetMidiData(FMidiNotesData::BuildFromMidiFile(LinkedMidiFile), SongsMap);
            VisualizationData = FMidiFileVisualizationData::BuildFromMidiFile(LinkedMidiFile);
        }
        else
        {
            PianorollWidget->SetMidiData(nullptr, nullptr);
            VisualizationData = FMidiFileVisualizationData();
        }
    }
}

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
        .Clipping(EWidgetClipping::ClipToBounds)
        .LinkedMidiData(MidiData)
        .LinkedSongsMap(SongsMap)
        // Bind to getter using lambda - will be called each frame during paint
        .VisualizationData(TAttribute<FMidiFileVisualizationData>::CreateLambda([this]() { return GetVisualizationData(); }))
		.TimeMode(TAttribute<EMidiTrackTimeMode>::CreateLambda([this]() { return GetTimeDisplayMode(); }))
        .PianorollStyle(&PianorollStyle)
        .Offset(FVector2D::ZeroVector)
        .Zoom(FVector2D(1.0f, 1.0f));

    return PianorollWidget.ToSharedRef();
}

void UMidiPianoroll::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    PianorollWidget.Reset();
}

void UMidiPianoroll::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
   
	if(PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMidiPianoroll, LinkedMidiFile))
    {
		SetMidiFile(LinkedMidiFile);
	}
}

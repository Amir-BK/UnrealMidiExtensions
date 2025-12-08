// Fill out your copyright notice in the Description page of Project Settings.


#include "MidiPianorollWidgetStyle.h"
#include "HarmonixMidi/MidiFile.h"

FMidiPianorollStyle::FMidiPianorollStyle()
{
}

FMidiPianorollStyle::~FMidiPianorollStyle()
{
}

const FName FMidiPianorollStyle::TypeName(TEXT("FMidiPianorollStyle"));

const FMidiPianorollStyle& FMidiPianorollStyle::GetDefault()
{
	static FMidiPianorollStyle Default;
	return Default;
}

void FMidiPianorollStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	// Add any brush resources here so that Slate can correctly atlas and reference them
}

FMidiFileVisualizationData FMidiFileVisualizationData::BuildFromMidiFile(UMidiFile* MidiFile)
{
	FMidiFileVisualizationData VisualizationData;
	if (MidiFile)
	{
		const int32 NumTracks = MidiFile->GetNumTracks();
		VisualizationData.TrackVisualizations.SetNum(NumTracks);
		for (int32 TrackIdx = 0; TrackIdx < NumTracks; ++TrackIdx)
		{
			FMidiTrackVisualizationData& TrackVisData = VisualizationData.TrackVisualizations[TrackIdx];
			// Default settings; can be customized later
			TrackVisData.bIsVisible = true;
			TrackVisData.TrackColor = FLinearColor::MakeRandomColor();
		}
	}
	return VisualizationData;
}
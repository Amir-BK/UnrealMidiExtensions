// Fill out your copyright notice in the Description page of Project Settings.


#include "MidiPianorollWidgetStyle.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Settings/MidiExtensionsDevSettings.h"

FMidiPianorollStyle::FMidiPianorollStyle()
{
	NoteBrush = FSlateRoundedBoxBrush(FStyleColors::White, 2.0f, FStyleColors::AccentBlack, 1.0f);
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


FMidiFileVisualizationData FMidiFileVisualizationData::BuildFromLinkedMidiData(FMidiNotesData& LinkedMidiData)
{
	FMidiFileVisualizationData VisualizationData;

	// Build visualization data from linked MIDI data
	int i = 0;
	for (const auto& NotesTrack : LinkedMidiData.Tracks)
	{
		FMidiTrackVisualizationData TrackVisData;
		TrackVisData.bIsVisible = true;
		
		// Use seeded random color for consistency across reloads
		const UMidiExtensionsDevSettings* DevSettings = GetDefault<UMidiExtensionsDevSettings>();
		if (DevSettings && DevSettings->DefaultTrackColors.IsValidIndex(i))
		{
			TrackVisData.TrackColor = DevSettings->DefaultTrackColors[i++];
		}
		else
		{
			TrackVisData.TrackColor = FLinearColor::IntToDistinctColor(i++);
		}
		
		TrackVisData.TrackName = FName(*NotesTrack.TrackName);
		TrackVisData.TrackIndex = NotesTrack.TrackIndex;
		TrackVisData.ChannelIndex = NotesTrack.ChannelIndex;
		VisualizationData.TrackVisualizations.Add(TrackVisData);
		i++;
	}

	return VisualizationData;
}

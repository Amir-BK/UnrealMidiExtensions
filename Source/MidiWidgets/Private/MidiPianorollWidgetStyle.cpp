// Fill out your copyright notice in the Description page of Project Settings.


#include "MidiPianorollWidgetStyle.h"
#include "HarmonixMidi/MidiFile.h"
#include "Brushes/SlateRoundedBoxBrush.h"

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

FMidiFileVisualizationData FMidiFileVisualizationData::BuildFromMidiFile(UMidiFile* MidiFile)
{
	FMidiFileVisualizationData VisualizationData;
	if (MidiFile)
	{
		// Discover unique (Track, Channel) combinations by scanning note events
		// This handles MIDI files where a single track contains notes on multiple channels
		TArray<TTuple<int32, int32>> FoundChannels;
		
		for (int32 TrackIdx = 0; TrackIdx < MidiFile->GetNumTracks(); ++TrackIdx)
		{
			const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
			if (!Track) continue;
			
			for (const FMidiEvent& Event : Track->GetEvents())
			{
				// Only consider note-on events to discover channels with actual notes
				if (Event.GetMsg().IsNoteOn())
				{
					const int32 Channel = Event.GetMsg().GetStdChannel();
					FoundChannels.AddUnique(TTuple<int32, int32>(TrackIdx, Channel));
				}
			}
		}
		
		// If no notes were found, fall back to creating one entry per track
		if (FoundChannels.IsEmpty())
		{
			for (int32 TrackIdx = 0; TrackIdx < MidiFile->GetNumTracks(); ++TrackIdx)
			{
				const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
				if (Track)
				{
					FoundChannels.Add(TTuple<int32, int32>(TrackIdx, Track->GetPrimaryMidiChannel()));
				}
			}
		}
		
		// Create visualization data for each discovered (Track, Channel) pair
		VisualizationData.TrackVisualizations.SetNum(FoundChannels.Num());
		for (int32 Idx = 0; Idx < FoundChannels.Num(); ++Idx)
		{
			const int32 TrackIdx = FoundChannels[Idx].Get<0>();
			const int32 ChannelIdx = FoundChannels[Idx].Get<1>();
			
			FMidiTrackVisualizationData& TrackVisData = VisualizationData.TrackVisualizations[Idx];
			
			TrackVisData.bIsVisible = true;
			// Use seeded random color for consistency across reloads
			TrackVisData.TrackColor = FLinearColor::MakeRandomSeededColor(ChannelIdx * 16 + TrackIdx);
			
			// Create descriptive name including channel if different from primary
			const FMidiTrack* Track = MidiFile->GetTrack(TrackIdx);
			const bool bIsPrimaryChannel = Track && (Track->GetPrimaryMidiChannel() == ChannelIdx);
			if (bIsPrimaryChannel && Track)
			{
				TrackVisData.TrackName = FName(*Track->GetName());
			}
			else
			{
				const FString TrackName = Track ? FString(*Track->GetName()) : FString(TEXT("Track"));
				TrackVisData.TrackName = FName(*FString::Printf(TEXT("%s Ch:%d"), *TrackName, ChannelIdx));
			}
			
			TrackVisData.TrackIndex = TrackIdx;
			TrackVisData.ChannelIndex = ChannelIdx;
		}
	}
	return VisualizationData;
}
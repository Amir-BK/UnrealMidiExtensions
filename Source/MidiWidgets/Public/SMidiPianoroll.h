// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "MidiFile/MidiNotesData.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "HarmonixMidi/MidiFile.h"
#include "HarmonixMidi/SongMaps.h"
#include "MidiPianorollWidgetStyle.h"

/**
 * 
 */
class MIDIWIDGETS_API SMidiPianoroll : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SMidiPianoroll, SCompoundWidget)

public:
    SLATE_BEGIN_ARGS(SMidiPianoroll)
	{}
           /** The MIDI data to visualize */
           SLATE_ARGUMENT(TSharedPtr<FMidiNotesData>, LinkedMidiData)
           /** The visualization data for the MIDI file - use ATTRIBUTE for live binding */
           SLATE_ATTRIBUTE(FMidiFileVisualizationData, VisualizationData)
        /** The song maps associated with the MIDI file */
        SLATE_ARGUMENT(TSharedPtr<FSongMaps>, LinkedSongsMap)
        /** The style set for this widget */
        SLATE_STYLE_ARGUMENT(FMidiPianorollStyle, PianorollStyle)
        /** Pan/offset in local space */
        SLATE_ATTRIBUTE(FVector2D, Offset)
        /** Zoom in X/Y */
        SLATE_ATTRIBUTE(FVector2D, Zoom)
	SLATE_END_ARGS()

	SMidiPianoroll();
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    const FMidiPianorollStyle* PianorollStyle = nullptr;

	

protected:
TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> LinkedMidiData;

TSharedPtr<FSongMaps, ESPMode::ThreadSafe> LinkedSongsMap;

// Changed from TSharedPtr to TSlateAttribute for live binding
TSlateAttribute<FMidiFileVisualizationData> VisualizationData;


	TSlateAttribute<FVector2D> Offset;

	TSlateAttribute<FVector2D> Zoom;
};

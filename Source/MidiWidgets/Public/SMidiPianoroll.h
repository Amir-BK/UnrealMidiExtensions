// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "MidiFile/MidiNotesData.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "HarmonixMidi/MidiFile.h"
#include "HarmonixMidi/SongMaps.h"
#include "MidiPianorollWidgetStyle.h"
#include "Misc/Optional.h"

/** Grid point type for timeline markings */
enum class EPianorollGridPointType : uint8
{
	Bar,
	Beat,
	Subdivision
};

/** Represents a single grid point on the timeline */
struct FPianorollGridPoint
{
	EPianorollGridPointType Type = EPianorollGridPointType::Bar;
	int32 Bar = 0;
	int8 Beat = 1;
	int8 Subdivision = 1;
};

/** Grid density levels - determines how much detail to show based on zoom */
enum class EPianorollGridDensity : uint8
{
	Subdivisions,	// Show bars, beats, and subdivisions
	Beats,			// Show bars and beats
	Bars,			// Show only bars
	SparseBars,		// Show every other bar
	VerySparseBars	// Show every 4th bar
};

/**
 * 
 */
class MIDIWIDGETS_API SMidiPianoroll : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SMidiPianoroll, SCompoundWidget)

public:
    SLATE_BEGIN_ARGS(SMidiPianoroll)
		: _TimelineHeight(25.0f)
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
		/** Time display mode */ 
		SLATE_ATTRIBUTE(EMidiTrackTimeMode, TimeMode)
		/** Height of the timeline bar at the top */
		SLATE_ARGUMENT(float, TimelineHeight)
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

	TSlateAttribute<EMidiTrackTimeMode> TimeMode;

	/** Height of the timeline header */
	float TimelineHeight = 25.0f;

	/** Cached grid points for timeline rendering */
	mutable TMap<int32, FPianorollGridPoint> GridPoints;

	/** Current grid density based on zoom level */
	mutable EPianorollGridDensity CurrentGridDensity = EPianorollGridDensity::Bars;

public:
	//SWidget interface
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	TOptional<EMouseCursor::Type> GetCursor() const override;

    void SetMidiData(TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> InMidiData, TSharedPtr<FSongMaps, ESPMode::ThreadSafe> InSongsMap = nullptr)
    {
        LinkedMidiData = InMidiData;
        if (InSongsMap.IsValid())
        {
            LinkedSongsMap = InSongsMap;
        }
	}


private:
	bool bIsPanning = false;
	bool bIsRightMouseButtonDown = false;
	bool bIsZooming = false;

	/** Uses the current zoom, the song map, and the time mode to convert a tick to a pixel position */
	double TickToPixel(double Tick) const;

	/** Converts a pixel position back to a tick value */
	double PixelToTick(double Pixel) const;

	/** Recalculates the grid points based on current zoom and visible range */
	void RecalculateGrid(const FGeometry& AllottedGeometry) const;

	/** Calculates the optimal grid density based on zoom level */
	EPianorollGridDensity CalculateOptimalGridDensity(float PixelsPerBar) const;

	/** Determines if a bar number should be shown at current density */
	bool ShouldShowBar(int32 BarNumber, EPianorollGridDensity Density) const;

	/** Paints the timeline header with bar numbers and grid lines */
	int32 PaintTimeline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	/** Paints vertical grid lines for bars/beats */
	int32 PaintGridLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	/** Calculates the total content height based on zoom */
	float GetContentHeight() const;

	/** Calculates the total content width based on MIDI data and zoom */
	float GetContentWidth() const;

	/** Clamps the offset to valid bounds based on viewport and content size */
	FVector2D ClampOffset(const FVector2D& InOffset, const FVector2D& ViewportSize) const;
};

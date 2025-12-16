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

struct FNotesEditCallbackData;



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
		/** Grid point type */
		SLATE_ATTRIBUTE(EPianorollGridPointType, GridPointType)
		/** Edit mode (select or paint) */
		SLATE_ATTRIBUTE(EPianorollEditMode, EditMode)
		/** Track index to edit (for painting) - use -1 for any visible track */
		SLATE_ATTRIBUTE(int32, EditingTrackIndex)
		/** Default velocity for painted notes */
		SLATE_ATTRIBUTE(int32, DefaultNoteVelocity)
		/** Default duration in ticks for painted notes */
		SLATE_ATTRIBUTE(int32, DefaultNoteDurationTicks)
		/** Is file editable (support selection, moving notes, etc.) */
		SLATE_ARGUMENT_DEFAULT(bool, bIsEditable) { false };
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

	TSlateAttribute<EPianorollGridPointType> GridPointType;

	TSlateAttribute<EPianorollEditMode> EditMode;

	TSlateAttribute<int32> EditingTrackIndex;

	TSlateAttribute<int32> DefaultNoteVelocity;

	TSlateAttribute<int32> DefaultNoteDurationTicks;

	bool bIsEditable;

	/** Height of the timeline header */
	float TimelineHeight = 25.0f;

	/** Cached grid points for timeline rendering */
	mutable TMap<int32, FPianorollGridPoint> GridPoints;

	/** Current grid density based on zoom level */
	mutable EPianorollGridDensity CurrentGridDensity = EPianorollGridDensity::Bars;

public:

	void SetIsEditable(bool bInIsEditable)
	{
		bIsEditable = bInIsEditable;
	}

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

// Marquee selection
bool bIsMarqueeSelecting = false;
FVector2D MarqueeStartPos;
FVector2D MarqueeCurrentPos;

// Note dragging state
bool bIsDraggingNotes = false;
FVector2D DragStartPos;
int32 DragStartTick = 0;
int32 DragStartNoteNumber = 0;

// Painting state
bool bIsPainting = false;


	
// Selected notes identified by track index and note index
struct FNoteIdentifier
{
int32 TrackIndex;
int32 NoteIndex;

bool operator==(const FNoteIdentifier& Other) const
{
return TrackIndex == Other.TrackIndex && NoteIndex == Other.NoteIndex;
}

friend uint32 GetTypeHash(const FNoteIdentifier& Key)
{
return HashCombine(GetTypeHash(Key.TrackIndex), GetTypeHash(Key.NoteIndex));
}
};
	
TSet<FNoteIdentifier> SelectedNotes;

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

	/** Performs marquee selection based on current marquee rectangle */
	void PerformMarqueeSelection(const FGeometry& MyGeometry, bool bAddToSelection);

	/** Checks if a note is currently selected */
	bool IsNoteSelected(int32 TrackIndex, int32 NoteIndex) const;

	/** Converts a screen Y position to a MIDI note number */
	int32 ScreenYToNoteNumber(float ScreenY, const FGeometry& AllottedGeometry) const;

	/** Finds a note at the given screen position, returns true if found */
	bool FindNoteAtPosition(const FVector2D& ScreenPos, const FGeometry& AllottedGeometry, int32& OutTrackIndex, int32& OutNoteIndex) const;

	/** Snaps a tick to the nearest grid point */
	int32 SnapTickToGrid(int32 Tick) const;

public:
	/** Gets the current selected notes */
	const TSet<FNoteIdentifier>& GetSelectedNotes() const { return SelectedNotes; }

	/** Clears all selected notes */
	void ClearSelection() { SelectedNotes.Empty(); }

	/** Delegate called when notes need to be modified */
	DECLARE_DELEGATE_OneParam(FOnNotesModified, const TArray<struct FNotesEditCallbackData>&);
	FOnNotesModified OnNotesModified;

	/** Delegate called when selected notes should be deleted */
	DECLARE_DELEGATE(FOnDeleteSelectedNotes);
	FOnDeleteSelectedNotes OnDeleteSelectedNotes;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual bool SupportsKeyboardFocus() const override { return true; }
};

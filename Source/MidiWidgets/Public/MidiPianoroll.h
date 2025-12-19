// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MidiPianorollWidgetStyle.h"
#include "SMidiPianoroll.h"
#include "MidiPianoroll.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class MIDIWIDGETS_API UMidiPianoroll : public UWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MIDI", BlueprintSetter = SetMidiFile)
	class UMidiFile* LinkedMidiFile;

	UPROPERTY(EditAnywhere, Category = "MIDI")
	FMidiFileVisualizationData VisualizationData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI")
	EMidiTrackTimeMode TimeDisplayMode = EMidiTrackTimeMode::TimeLinear;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI")
	EPianorollGridPointType GridPointType = EPianorollGridPointType::Subdivision;

	/** Grid subdivision for visual grid lines (when GridPointType is Subdivision) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI")
	EMidiClockSubdivisionQuantization GridSubdivision = EMidiClockSubdivisionQuantization::SixteenthNote;

	/** The track index to edit when in paint mode. Set to -1 to allow editing any visible track. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", BlueprintSetter = SetEditingTrackIndex, meta = (ClampMin = "-1", EditCondition = "IsEditable"))
	int32 EditingTrackIndex = 0;

	/** The current edit mode for the piano roll */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", BlueprintSetter = SetEditMode, meta=(EditCondition = "IsEditable"))
	EPianorollEditMode EditMode = EPianorollEditMode::Select;

	/** Default velocity for painted notes (0-127) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", BlueprintSetter = SetDefaultNoteVelocity, meta = (ClampMin = "1", ClampMax = "127", EditCondition = "IsEditable"))
	int32 DefaultNoteVelocity = 100;

	/** Default note duration in ticks for painted notes (fallback if no song map) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", BlueprintSetter = SetDefaultNoteDurationTicks, meta = (ClampMin = "1", EditCondition = "IsEditable"))
	int32 DefaultNoteDurationTicks = 480;

	/** Note snapping quantization for editing (snap note start positions) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", meta = (EditCondition = "IsEditable"))
	EMidiClockSubdivisionQuantization NoteSnapping = EMidiClockSubdivisionQuantization::SixteenthNote;

	/** Note duration quantization for painted notes */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MIDI|Editing", meta = (EditCondition = "IsEditable"))
	EMidiClockSubdivisionQuantization NoteDuration = EMidiClockSubdivisionQuantization::SixteenthNote;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FMidiPianorollStyle PianorollStyle;

	UFUNCTION(BlueprintSetter)
	void SetMidiFile(UMidiFile* InMidiFile);

	UFUNCTION(BlueprintSetter)
	void SetEditingTrackIndex(int32 InTrackIndex);

	UFUNCTION(BlueprintSetter)
	void SetEditMode(EPianorollEditMode InEditMode);

	UFUNCTION(BlueprintSetter)
	void SetDefaultNoteVelocity(int32 InVelocity);

	UFUNCTION(BlueprintSetter)
	void SetDefaultNoteDurationTicks(int32 InDurationTicks);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "MIDI")
	void MakeEditableCopyOfLinkedMidiFile();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "MIDI")
	void SaveMidiFileToAsset();

	/** Delete all currently selected notes */
	UFUNCTION(BlueprintCallable, Category = "MIDI|Editing")
	void DeleteSelectedNotes();

	/** Clear the current note selection */
	UFUNCTION(BlueprintCallable, Category = "MIDI|Editing")
	void ClearSelection();

	/** Get the number of currently selected notes */
	UFUNCTION(BlueprintCallable, Category = "MIDI|Editing")
	int32 GetSelectedNoteCount() const;

	/** Check if the piano roll is in editable mode */
	UFUNCTION(BlueprintCallable, Category = "MIDI|Editing")
	bool IsEditable() const;

	/** Save the current MIDI file as a new asset */
	UFUNCTION(BlueprintCallable, Category = "MIDI")
	class UMidiFile* SaveMidiFileAsAsset(const FString& PackagePath, const FString& AssetName);

	// Getter for attribute binding
	const FMidiFileVisualizationData& GetVisualizationData() { return VisualizationData; }

	const EMidiTrackTimeMode GetTimeDisplayMode() const { return TimeDisplayMode; }

	const EPianorollGridPointType GetGridPointType() const { return GridPointType; }

	int32 GetEditingTrackIndex() const { return EditingTrackIndex; }

	EPianorollEditMode GetEditMode() const { return EditMode; }

	int32 GetDefaultNoteVelocity() const { return DefaultNoteVelocity; }

	int32 GetDefaultNoteDurationTicks() const { return DefaultNoteDurationTicks; }

	EMidiClockSubdivisionQuantization GetGridSubdivision() const { return GridSubdivision; }

	EMidiClockSubdivisionQuantization GetNoteSnapping() const { return NoteSnapping; }

	EMidiClockSubdivisionQuantization GetNoteDuration() const { return NoteDuration; }

	TSharedRef<SWidget> RebuildWidget() override;

	void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	TSharedPtr<SMidiPianoroll> PianorollWidget;
};

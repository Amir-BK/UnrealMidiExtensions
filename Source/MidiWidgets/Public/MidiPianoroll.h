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

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FMidiPianorollStyle PianorollStyle;

	UFUNCTION(BlueprintSetter)
	void SetMidiFile(UMidiFile* InMidiFile);

	// Getter for attribute binding
	const FMidiFileVisualizationData& GetVisualizationData() { return VisualizationData; }

	const EMidiTrackTimeMode GetTimeDisplayMode() const { return TimeDisplayMode; }

	const EPianorollGridPointType GetGridPointType() const { return GridPointType; }

	TSharedRef<SWidget> RebuildWidget() override;

	void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	TSharedPtr<SMidiPianoroll> PianorollWidget;
};

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
class MIDIWIDGETS_API UMidiPianoroll : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MIDI")
	class UMidiFile* LinkedMidiFile;

	UPROPERTY(EditAnywhere, Category = "MIDI")
	FMidiFileVisualizationData VisualizationData;

	TSharedRef<SWidget> RebuildWidget() override;

	void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SMidiPianoroll> PianorollWidget;
};

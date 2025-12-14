// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MidiExtensionsDevSettings.generated.h"

/**
 * 
 */
UCLASS(config = Game)
class MIDIEXTENSIONS_API UMidiExtensionsDevSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UMidiExtensionsDevSettings();
	UPROPERTY(EditAnywhere, Config, Category = "MIDI Extensions|Cosmetics")
	TArray<FLinearColor> DefaultTrackColors;
};

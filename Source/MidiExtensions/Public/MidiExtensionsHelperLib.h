// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MidiFile/MidiNotesData.h"
#include "MidiExtensionsHelperLib.generated.h"

/**
 * 
 */
UCLASS()
class MIDIEXTENSIONS_API UMidiExtensionsHelperLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintPure, Category = "MIDI Extensions|Utils")
	static FMidiFileIterator MakeMidiFileIterator(FMidiNotesData MidiDataPtr);
};

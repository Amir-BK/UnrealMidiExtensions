// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Framework/Commands/UICommandList.h"
#include "MidiPianorollPalette.generated.h"

/**
 * 
 */
UCLASS()
class MIDIWIDGETS_API UMidiPianorollPalette : public UWidget
{
	GENERATED_BODY()

public:

	TSharedRef<FUICommandList> GetPaletteCommandList();

	void BindCommands();

	TSharedRef<SWidget> RebuildWidget() override;
	
};

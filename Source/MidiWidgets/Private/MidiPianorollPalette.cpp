// Fill out your copyright notice in the Description page of Project Settings.


#include "MidiPianorollPalette.h"
#include "Widgets/SNullWidget.h"

TSharedRef<FUICommandList> UMidiPianorollPalette::GetPaletteCommandList()
{
	static TSharedRef<FUICommandList> CommandList = MakeShared<FUICommandList>();
	return CommandList;
}

void UMidiPianorollPalette::BindCommands()
{
	// TODO: Implement command bindings
}

TSharedRef<SWidget> UMidiPianorollPalette::RebuildWidget()
{
	// TODO: Implement palette widget
	return SNullWidget::NullWidget;
}


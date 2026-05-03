// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreasureHuntGameMode.h"
#include "TreasureHuntCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATreasureHuntGameMode::ATreasureHuntGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

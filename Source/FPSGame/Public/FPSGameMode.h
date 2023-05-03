// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

UENUM(BluePrintType)
enum class EEnemytype:uint8{
	ELeft,
	ERight,
	ENone
};

UENUM(BluePrintType)
enum class EFPSProjectiletype :uint8 {
	ELeft,
	ERight,
	ENone
};
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Physical,
	Fire,
	Ice,
	Electric,
};

UCLASS()
class AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AFPSGameMode();

	protected:
		virtual void BeginPlay() override;
};




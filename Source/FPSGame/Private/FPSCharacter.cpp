// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "FPSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FPSBombActor.h"
#include "DrawDebugHelpers.h"
#include "BombDamageType.h"
#include "Kismet/KismetMathLibrary.h"

void AFPSCharacter::TakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Green, "Damage Received - " + FString::FromInt(Damage));

	const UBombDamageType* damageType = Cast<UBombDamageType>(DamageType);
	if (damageType)
	{
		GetCapsuleComponent()->SetSimulatePhysics(false);
		DisableInput(nullptr);
	}

}

AFPSCharacter::AFPSCharacter()
{
	// Create a CameraComponent	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	CameraComponent->SetRelativeLocation (FVector(0, 0, BaseEyeHeight)); // Position the camera
	CameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1PComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh1PComponent->SetupAttachment(CameraComponent);
	Mesh1PComponent->CastShadow = false;
	Mesh1PComponent->SetRelativeRotation (FRotator(2.0f, -15.0f, 5.0f));
	Mesh1PComponent->SetRelativeLocation ( FVector(0, 0, -160.0f));

	// Create a gun mesh component
	GunMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	GunMeshComponent->CastShadow = false;
	GunMeshComponent->SetupAttachment(Mesh1PComponent, "GripPoint");

	//Subscribe to Damage Event
	OnTakeAnyDamage.AddDynamic(this, &AFPSCharacter::TakeAnyDamage);

	//SET HeldBomb to null
	HeldBomb = nullptr;
}


void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCharacter::Fire);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("SpawnBomb", IE_Pressed, this, &AFPSCharacter::PickupBomb);
	PlayerInputComponent->BindAction("SpawnBomb", IE_Released, this, &AFPSCharacter::ThrowBomb);

}

void AFPSCharacter::Fire()
{
	// try and fire a projectile
	if (ProjectileClass)
	{
		FVector MuzzleLocation = GunMeshComponent->GetSocketLocation("Muzzle");
		FRotator MuzzleRotation = GunMeshComponent->GetSocketRotation("Muzzle");

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		// spawn the projectile at the muzzle
		GetWorld()->SpawnActor<AFPSProjectile>(ProjectileClass, MuzzleLocation, MuzzleRotation, ActorSpawnParams);
	}

	// try and play the sound if specified
	PlaySound();

	// try and play a firing animation if specified
	PlayAnimation();
}

void AFPSCharacter::PlaySound()
{
	// try and play the sound if specified
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

}

void AFPSCharacter::PlayAnimation()
{
	if (FireAnimation)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1PComponent->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Arms", 0.0f);
		}
	}
}

void AFPSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSCharacter::SetupRay(FVector& StartTrace, FVector& Direction, FVector& EndTrace)
{
	FVector CamLoc;
	FRotator CamRot;

	Controller->GetPlayerViewPoint(CamLoc, CamRot); // Get the camera position and rotation

	StartTrace = CamLoc; // trace start is the camera location
	Direction = CamRot.Vector();
	EndTrace = StartTrace + Direction * 300;
}

AActor* AFPSCharacter::RayCastGetActor()
{
	if (Controller && Controller->IsLocalPlayerController()) // we check the controller because we don't want bots to grab the use object and we need a controller for the GetPlayerViewpoint function
	{
		FVector StartTrace;
		FVector Direction;
		FVector EndTrace;

		SetupRay(StartTrace, Direction, EndTrace);

		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);
		TraceParams.bTraceComplex = true;
		TraceParams.bReturnPhysicalMaterial = true;

		FHitResult Hit(ForceInit);
		UWorld* World = GetWorld();
		//World->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, ObjectTypeQuery1, TraceParams); // simple trace function  ECC_PhysicsBody
		World->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_PhysicsBody, TraceParams); // simple trace function  ECC_PhysicsBody
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Green, false, 2, 0, 1.f);
		return Hit.GetActor();
	}
	return nullptr;
}

void AFPSCharacter::PickupBomb()
{
	//Ray Cast to check if a Actor is hit by a raycast
	AActor* PickedActor = RayCastGetActor();
	//IF a Actor has been detected/hit
	if (PickedActor)
	{
		//Check if it is a AFPSBombActor by doing a cast
		AFPSBombActor* bomb = Cast<AFPSBombActor>(PickedActor);
		//IF it is a AFPSBombActor
		if (bomb)
		{
			//SET HeldBomb to the hit bomb
			HeldBomb = bomb;
			//CALL Hold on the bomb passing in GunMeshComponent, we will attach it to the Gun
			bomb->Hold(GunMeshComponent);
		}
		//ENDIF
		//
	}
	//ENDIF
}

void AFPSCharacter::ThrowBomb()
{
	//IF we have a HeldBomb
	if (HeldBomb)
	{
		//Throw the Held Bomb passing in the Camera's Forward Vector
		HeldBomb->Throw(CameraComponent->GetForwardVector());
		//Nullify HeldBomb
		HeldBomb = nullptr;
	}
	//ENDIF
}

FQuat AFPSCharacter::RotateDirection(FRotator Rotation, FVector Direction)
{
	FQuat RootQuat = Rotation.Quaternion();
	FVector UpVector = RootQuat.GetUpVector();
	FVector RotationAxis = FVector::CrossProduct(UpVector, Direction);
	RotationAxis.Normalize();
	float DotProduct = FVector::DotProduct(UpVector, Direction);
	float RotationAngle = acosf(DotProduct);
	FQuat Quat = FQuat(RotationAxis, RotationAngle);
	FQuat NewQuat = Quat * RootQuat;

	return NewQuat;
}


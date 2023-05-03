// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FPSProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

AFPSProjectile::AFPSProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AFPSProjectile::OnHit);	// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}


void AFPSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
	    //Add Impulse to the Other Component to ...
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
		//CALL SetRandomColor() Pass In OtherComp, FLinearColor::MakeRandomColor()
		SetRandomColor(OtherComp, FLinearColor::MakeRandomColor());
		//CALL SetScale, Pass in OtherActor, OtherComp
		SetScale(OtherActor, OtherComp);
		//DESTROY this Actor
		Destroy();
	}
}
void AFPSProjectile::SetScale(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	//Get the Hit Actors Scale
	FVector Scale = OtherComp->GetComponentScale();
	//Scale it by 0.8 or any factor you want ??
	Scale *= 0.8f;

	//IF the Scale is less than 0.5
	if (Scale.GetMin() < 0.5f)
	{
		//Destroy the Hit Actor
		OtherActor->Destroy();

	}
	//ELSE
	else
	{
		//Scale the Hit Actor by Scale Value
		OtherComp->SetWorldScale3D(Scale);
	}
	//ENDIF
}
void AFPSProjectile::SetRandomColor(UPrimitiveComponent* OtherComp, FLinearColor color)
{
	//Now we will create a dynamic material instance that is the mesh surface. we will name it as MAtinst
		//CREATE a Dynamic Material Instance for the specified element index. 
		//The function to use is:
		//UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(int32 ElementIndex)
		//Look it up in the documentation and on how and where to use it	
	UMaterialInstanceDynamic* MatInst = OtherComp->CreateAndSetMaterialInstanceDynamic(0);
	//IF the MatInst is successful
	if (MatInst)
	{
		//Set the MatInst to a RANDOM color using the function:
		//void SetVectorParameterValue(FName ParameterName, FLinearColor Value)
		//The parameter name is Color
		//Look up documentation on FLinearColor
		MatInst->SetVectorParameterValue("Color", FLinearColor::MakeRandomColor());
	}
	//ENDIF
}

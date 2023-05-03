// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSBombActor.h"
#include "kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "GameFramework/DamageType.h"
#include "BombDamageType.h"
// Sets default values
AFPSBombActor::AFPSBombActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//CREATE the BombBox Component
	BombBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SphereComp"));
	//SET the BombBox's profile name to BlockAllDynamic
	BombBox->SetCollisionProfileName("BlockAllDynamic");
	//SUBSCRIBE to the BombBox's Hit event using the function OnHit()
	BombBox->OnComponentHit.AddDynamic(this, &AFPSBombActor::OnHit);	// set up a notification for when this component hits something blocking

	// Players can't walk on it
	//CALL SetWalkableSlopeOverride() on BombBox passing in WalkableSlope_Unwalkable, 0.f)
	BombBox->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	//SET CanCharacterStepUpOn on BombBox to ECB_No
	BombBox->CanCharacterStepUpOn = ECB_No;

	//SET BombBox as the RootComponent
	RootComponent = BombBox;

	//CREATE the BombMesh
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>("BombMesh");
	//ATTACH it to the RootComponent
	BombMesh->SetupAttachment(RootComponent);
	//CALL SetCollisionEnabled() on BombMesh passing in ECollisionEnabled::NoCollision
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//CREATE the RadialForceComp
	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	//ATTACH the RadialForceComp to the RootComponent
	RadialForceComp->SetupAttachment(RootComponent);
	//SET the RadialForceComp's Radius to 250 or a value of your choice
	RadialForceComp->Radius = 250;
	//SET the RadialForceComp's bImpulseVelChange property to true
	RadialForceComp->bImpulseVelChange = true;
	//SET the RadialForceComp's bAutoActivate property to false. This will prevent the RadialForceComp from ticking and we will only use FireImpulse() instead
	RadialForceComp->bAutoActivate = false; // Prevent component from ticking, and only use FireImpulse() instead
	//SET the RadialForceComp's bIgnoreOwningActor property to true (ignoring self)
	RadialForceComp->bIgnoreOwningActor = true; // ignore self


	//CALL SetSimulatePhysics() on the BombBox to true
	BombBox->SetSimulatePhysics(true);
	//CALL SetCollisionEnabled() on BombBox and enable it to respond to Queries and Physics Collision
	BombBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//CALL SetCollisionResponseToChannel() on the BombBox, Make it Ignore the Pawn
	BombBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); //Was ECR_Ignore

	//SET ExplodeDelay to 2.0f
	ExplodeDelay = 2.0f;

}

void AFPSBombActor::DestroyBomb()
{
	//Destroy this Actor
	Destroy();
}

// Called when the game starts or when spawned
void AFPSBombActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFPSBombActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}
void AFPSBombActor::Hold(USkeletalMeshComponent* HoldingComponent)
{
	//IF HoldingComponent
	if (HoldingComponent)
	{
		//DISABLE Physics
		BombBox->SetSimulatePhysics(false);
		//ATTACH the BombBox to the HoldingComponent at the HoldingComponents Muzzle
		BombBox->AttachToComponent(HoldingComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("Muzzle"));

	}
	//ENDIF
}

void AFPSBombActor::Throw(FVector direction)
{
	//Enable Sphysics
	BombBox->SetSimulatePhysics(true);
	//CALL SetNotifyRigidBodyCollision() on BombBox and pass in true
	BombBox->SetNotifyRigidBodyCollision(true);
	//CALL SetCollisionEnabled() on BombBox passing in ??::QueryAndPhysics
	BombBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//CALL SetCollisionResponseToChannel() on BombBox and make it Block with Pawn
	BombBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	//DETACH BombBox
	BombBox->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//CALL AddForce(...) to BombBox to throw it, using the direction passed in
	BombBox->GetBodyInstance()->AddForce(direction * 100000 * BombBox->GetMass());
}

void AFPSBombActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	
	//CREATE a Timer to Explode this Actor After ExplodeDelay seconds
	FTimerHandle Explode_TimerHandle;
	GetWorldTimerManager().SetTimer(Explode_TimerHandle, this, &AFPSBombActor::Explode, ExplodeDelay, false);
}

void AFPSBombActor::Explode()
{
	//SPAWN the ExplosionTemplate using UGameplayStatics::SpawnEmitterAtLocation(....)
	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionTemplate, GetActorLocation());

	//FVector BoostIntensity = FVector::UpVector * ExplosionImpulse;
	//MeshComp->AddImpulse(BoostIntensity, NAME_None, true);

	// Blast away nearby physics actors by Calling FireImpulse() on the Radial Force Component
	RadialForceComp->FireImpulse();

	//BONUS: APPLY RADIAL DAMAGE
	//UGameplayStatics::ApplyRadialDamage(this, 100, GetActorLocation(), 250, UDamageType::StaticClass(), TArray<AActor*>(), this);
	UGameplayStatics::ApplyRadialDamage(this, 100, GetActorLocation(), RadialForceComp->Radius, UDamageType::StaticClass(), TArray<AActor*>(), this);

	//CREATE a Timer to Destroy this Actor After 1 second
	FTimerHandle Destroy_TimerHandle;
	GetWorldTimerManager().SetTimer(Destroy_TimerHandle, this, &AFPSBombActor::DestroyBomb, 1, false);

}




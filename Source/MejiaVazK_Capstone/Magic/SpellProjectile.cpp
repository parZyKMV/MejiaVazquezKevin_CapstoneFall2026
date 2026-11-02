#include "Magic/SpellProjectile.h"

#include "Combat/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ASpellProjectile::ASpellProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(16.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	SetRootComponent(Collision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 1800.f;
	Movement->MaxSpeed = 1800.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 4.f;
}

void ASpellProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASpellProjectile::HandleOverlap);
	Collision->OnComponentHit.AddDynamic(this, &ASpellProjectile::HandleHit);
}

void ASpellProjectile::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bImpacted || !OtherActor || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		return; // solo reacciona a lo que tiene vida
	}

	bImpacted = true;

	UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());

	if (StunDuration > 0.f)
	{
		Health->ApplyHitStun(StunDuration);
	}

	OnImpact(OtherActor, GetActorLocation());
	Destroy();
}

void ASpellProjectile::HandleHit(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult& Hit)
{
	if (bImpacted)
	{
		return;
	}

	bImpacted = true;
	OnImpact(nullptr, Hit.ImpactPoint);
	Destroy();
}

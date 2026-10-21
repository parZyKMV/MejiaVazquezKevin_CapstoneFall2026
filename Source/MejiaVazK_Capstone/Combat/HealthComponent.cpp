#include "HealthComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.f)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - OldHealth, DamageCauser);

	if (CurrentHealth <= 0.f)
	{
		Die(DamageCauser);
	}
}

void UHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - OldHealth, nullptr);
}

void UHealthComponent::Revive(float HealthPercent)
{
	bIsDead = false;
	ReleaseFromStun();

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(MaxHealth * HealthPercent, 1.f, MaxHealth);

	// Delta positivo: la barra se actualiza y el combo no se interrumpe.
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - OldHealth, nullptr);
}

void UHealthComponent::ApplyHitStun(float Duration)
{
	if (bIsDead || Duration <= 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!bIsStunned)
	{
		bIsStunned = true;
		OnStunStarted.Broadcast();
	}

	// Cada golpe renueva el tiempo de aturdimiento.
	World->GetTimerManager().SetTimer(StunTimerHandle, this, &UHealthComponent::ReleaseFromStun, Duration, false);
}

void UHealthComponent::ReleaseFromStun()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StunTimerHandle);
	}

	if (!bIsStunned)
	{
		return;
	}

	bIsStunned = false;
	OnStunEnded.Broadcast();
}

void UHealthComponent::Die(AActor* Killer)
{
	bIsDead = true;
	ReleaseFromStun();
	OnDeath.Broadcast(GetOwner(), Killer);
}
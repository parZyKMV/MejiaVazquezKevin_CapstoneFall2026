#include "Magic/ManaComponent.h"

#include "Engine/World.h"

UManaComponent::UManaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UManaComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentMana = MaxMana;
	SetComponentTickEnabled(false);
}

bool UManaComponent::TrySpend(float Cost)
{
	if (Cost <= 0.f)
	{
		return true;
	}

	if (!HasEnough(Cost))
	{
		return false;
	}

	ChangeMana(-Cost);

	if (UWorld* World = GetWorld())
	{
		RegenBlockedUntil = World->GetTimeSeconds() + RegenDelay;
	}
	SetComponentTickEnabled(true);

	if (CurrentMana <= 0.f)
	{
		OnManaDepleted.Broadcast();
	}

	return true;
}

void UManaComponent::Restore(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	ChangeMana(Amount);
}

void UManaComponent::ChangeMana(float Delta)
{
	const float OldMana = CurrentMana;
	CurrentMana = FMath::Clamp(OldMana + Delta, 0.f, MaxMana);

	const float RealDelta = CurrentMana - OldMana;
	if (!FMath::IsNearlyZero(RealDelta))
	{
		OnManaChanged.Broadcast(CurrentMana, MaxMana, RealDelta);
	}
}

void UManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < RegenBlockedUntil)
	{
		return;
	}

	if (RegenPerSecond <= 0.f || CurrentMana >= MaxMana)
	{
		SetComponentTickEnabled(false);
		return;
	}

	ChangeMana(RegenPerSecond * DeltaTime);

	if (CurrentMana >= MaxMana)
	{
		SetComponentTickEnabled(false);
	}
}

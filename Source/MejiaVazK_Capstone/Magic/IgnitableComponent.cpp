#include "Magic/IgnitableComponent.h"

#include "Engine/World.h"

UIgnitableComponent::UIgnitableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIgnitableComponent::BeginPlay()
{
	Super::BeginPlay();

	// El estado inicial se lee con IsLit() desde el BeginPlay del Blueprint del actor.
	bLit = bStartLit;
	if (bLit)
	{
		StartBurnTimer();
	}
}

void UIgnitableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurnTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UIgnitableComponent::StartBurnTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(BurnTimer);
	if (BurnDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(BurnTimer, this, &UIgnitableComponent::Extinguish, BurnDuration, false);
	}
}

bool UIgnitableComponent::Ignite(AActor* Igniter)
{
	if (bLit)
	{
		StartBurnTimer(); // ya encendido: solo renueva el tiempo
		return false;
	}

	bLit = true;
	StartBurnTimer();
	OnIgnited.Broadcast(Igniter);
	return true;
}

void UIgnitableComponent::Extinguish()
{
	if (!bLit)
	{
		return;
	}

	bLit = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurnTimer);
	}
	OnExtinguished.Broadcast();
}

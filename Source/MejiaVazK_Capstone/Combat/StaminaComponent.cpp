#include "Combat/StaminaComponent.h"

#include "Combat/CombatComponent.h"
#include "Combat/DodgeComponent.h"
#include "Combat/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	Stamina = MaxStamina;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			BaseWalkSpeed = Move->MaxWalkSpeed;
		}
	}

	if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
	{
		Health->OnHealthChanged.AddDynamic(this, &UStaminaComponent::HandleOwnerHealthChanged);
	}

	OnStaminaChanged.Broadcast(Stamina, MaxStamina, 0.f);
	SetComponentTickEnabled(false);
}

void UStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Devuelve la velocidad normal por si el componente desaparece corriendo.
	SetSprintActive(false);
	Super::EndPlay(EndPlayReason);
}

void UStaminaComponent::SetSprintInput(bool bHeld)
{
	bSprintHeld = bHeld;
	if (bHeld)
	{
		SetComponentTickEnabled(true);
	}
}

bool UStaminaComponent::TrySpend(float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}
	if (Stamina < Amount)
	{
		return false;
	}

	ChangeStamina(-Amount);
	return true;
}

void UStaminaComponent::Restore(float Amount)
{
	if (Amount > 0.f)
	{
		ChangeStamina(Amount);
	}
}

void UStaminaComponent::RestoreFull()
{
	bExhausted = false;
	ChangeStamina(MaxStamina);
}

void UStaminaComponent::ChangeStamina(float Delta)
{
	const float Old = Stamina;
	Stamina = FMath::Clamp(Stamina + Delta, 0.f, MaxStamina);
	const float Applied = Stamina - Old;

	if (Applied < 0.f)
	{
		if (const UWorld* World = GetWorld())
		{
			LastUseTime = World->GetTimeSeconds();
		}

		if (Stamina <= 0.f && !bExhausted)
		{
			bExhausted = true;
			bSprintHeld = false; // hay que soltar y volver a pulsar la tecla
			OnStaminaDepleted.Broadcast();
		}
	}

	if (!FMath::IsNearlyZero(Applied))
	{
		OnStaminaChanged.Broadcast(Stamina, MaxStamina, Applied);
	}

	if (Stamina < MaxStamina)
	{
		SetComponentTickEnabled(true);
	}
}

bool UStaminaComponent::ShouldSprint() const
{
	if (!bSprintHeld || bExhausted || bOwnerDead || Stamina <= 0.f)
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Move || !Move->IsMovingOnGround())
	{
		return false;
	}

	if (Move->Velocity.SizeSquared2D() < FMath::Square(MinSprintSpeed))
	{
		return false;
	}

	// Atacando o esquivando no se corre (ni se gasta).
	if (const UCombatComponent* Combat = Character->FindComponentByClass<UCombatComponent>())
	{
		if (Combat->GetCombatState() == ECombatState::Attacking)
		{
			return false;
		}
	}
	if (const UDodgeComponent* Dodge = Character->FindComponentByClass<UDodgeComponent>())
	{
		if (Dodge->IsDodging())
		{
			return false;
		}
	}

	return true;
}

void UStaminaComponent::SetSprintActive(bool bActive)
{
	if (bSprintActive == bActive)
	{
		return;
	}
	bSprintActive = bActive;

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->MaxWalkSpeed = bActive ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed;
		}
	}

	OnSprintStateChanged.Broadcast(bActive);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SetSprintActive(ShouldSprint());

	if (bSprintActive)
	{
		ChangeStamina(-SprintDrainPerSecond * DeltaTime);
		if (Stamina <= 0.f)
		{
			SetSprintActive(false);
		}
	}
	else if (Stamina < MaxStamina && World->GetTimeSeconds() - LastUseTime >= RegenDelay)
	{
		ChangeStamina(RegenPerSecond * DeltaTime);
	}

	if (bExhausted && Stamina >= SprintResumeThreshold)
	{
		bExhausted = false;
	}

	// Barra llena y sin correr: se apaga el Tick.
	if (!bSprintHeld && !bSprintActive && Stamina >= MaxStamina)
	{
		SetComponentTickEnabled(false);
	}
}

void UStaminaComponent::HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	if (NewHealth <= 0.f)
	{
		bOwnerDead = true;
		bSprintHeld = false;
		return;
	}

	// Revivir (reaparecer) devuelve la stamina completa.
	if (bOwnerDead && Delta > 0.f)
	{
		bOwnerDead = false;
		RestoreFull();
	}
}

#include "Combat/DodgeComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatComponent.h"
#include "Combat/HealthComponent.h"
#include "Combat/StaminaComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Magic/SpellComponent.h"

UDodgeComponent::UDodgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDodgeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
		{
			CachedHealth = Health;
			Health->OnHealthChanged.AddDynamic(this, &UDodgeComponent::HandleOwnerHealthChanged);
		}
	}
}

void UDodgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UDodgeComponent::Fail(EDodgeFailReason Reason)
{
	OnDodgeFailed.Broadcast(Reason);
}

FVector UDodgeComponent::GetDodgeDirection(const ACharacter* Character) const
{
	FVector Direction = FVector::ZeroVector;
	if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Direction = Move->GetCurrentAcceleration();
	}
	Direction.Z = 0.f;

	// Sin direccion de input: esquiva hacia adelante.
	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetActorForwardVector();
		Direction.Z = 0.f;
	}
	return Direction.GetSafeNormal();
}

void UDodgeComponent::SetInvulnerable(bool bValue)
{
	if (UHealthComponent* Health = CachedHealth.Get())
	{
		Health->bInvulnerable = bValue;
	}
}

bool UDodgeComponent::Dodge()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World || bDodging)
	{
		return false;
	}

	if (const UHealthComponent* Health = CachedHealth.Get())
	{
		if (Health->IsDead())
		{
			Fail(EDodgeFailReason::Dead);
			return false;
		}
	}

	if (World->GetTimeSeconds() < NextDodgeTime)
	{
		Fail(EDodgeFailReason::OnCooldown);
		return false;
	}

	// No se esquiva a mitad de un ataque de espada ni de un lanzamiento.
	const UCombatComponent* Combat = Character->FindComponentByClass<UCombatComponent>();
	const USpellComponent* Spells = Character->FindComponentByClass<USpellComponent>();
	if ((Combat && Combat->GetCombatState() == ECombatState::Attacking) || (Spells && Spells->IsCasting()))
	{
		Fail(EDodgeFailReason::Busy);
		return false;
	}

	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!bAllowInAir && Move && !Move->IsMovingOnGround())
	{
		Fail(EDodgeFailReason::InAir);
		return false;
	}

	// Ultimo paso: gastar la stamina (si falla antes, no se pierde nada).
	if (UStaminaComponent* Stamina = Character->FindComponentByClass<UStaminaComponent>())
	{
		if (!Stamina->TrySpend(StaminaCost))
		{
			Fail(EDodgeFailReason::NotEnoughStamina);
			return false;
		}
	}

	// Gira hacia la direccion del esquive y suelta el control de giro mientras dure.
	const FVector Direction = GetDodgeDirection(Character);
	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));

	bSavedUseControllerYaw = Character->bUseControllerRotationYaw;
	bSavedOrientToMovement = Move ? Move->bOrientRotationToMovement : false;
	Character->bUseControllerRotationYaw = false;
	if (Move)
	{
		Move->bOrientRotationToMovement = false;
	}

	bDodging = true;
	++DodgeSerial;

	SetInvulnerable(true);
	World->GetTimerManager().SetTimer(IFrameTimer, this, &UDodgeComponent::EndInvulnerability, FMath::Max(IFrameDuration, 0.01f), false);

	// Animacion (root motion) o, sin montage, un impulso corto.
	bool bUsingMontage = false;
	if (DodgeMontage && Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			if (AnimInstance->Montage_Play(DodgeMontage, PlayRate) > 0.f)
			{
				bUsingMontage = true;

				const int32 Serial = DodgeSerial;
				TWeakObjectPtr<UDodgeComponent> WeakThis(this);
				FOnMontageEnded EndDelegate;
				EndDelegate.BindLambda([WeakThis, Serial](UAnimMontage*, bool)
				{
					if (UDodgeComponent* Self = WeakThis.Get())
					{
						Self->HandleMontageEnded(Serial);
					}
				});
				AnimInstance->Montage_SetEndDelegate(EndDelegate, DodgeMontage);
			}
		}
	}

	if (!bUsingMontage)
	{
		Character->LaunchCharacter(Direction * FallbackDashSpeed, true, false);
		World->GetTimerManager().SetTimer(FallbackTimer, this, &UDodgeComponent::FinishDodge, FallbackDuration, false);
	}

	OnDodgeStarted.Broadcast(Direction);
	return true;
}

void UDodgeComponent::EndInvulnerability()
{
	SetInvulnerable(false);
}

void UDodgeComponent::HandleMontageEnded(int32 Serial)
{
	// Ignora finales de esquives anteriores.
	if (bDodging && Serial == DodgeSerial)
	{
		FinishDodge();
	}
}

void UDodgeComponent::FinishDodge()
{
	if (!bDodging)
	{
		return;
	}

	bDodging = false;
	++DodgeSerial; // invalida el final de montage pendiente

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(IFrameTimer);
		World->GetTimerManager().ClearTimer(FallbackTimer);
		NextDodgeTime = World->GetTimeSeconds() + Cooldown;
	}

	SetInvulnerable(false);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->bUseControllerRotationYaw = bSavedUseControllerYaw;
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = bSavedOrientToMovement;
		}
	}

	OnDodgeEnded.Broadcast();
}

void UDodgeComponent::HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	// Si muere a mitad de un esquive (por ejemplo una caida), se limpia el estado.
	if (NewHealth <= 0.f && bDodging)
	{
		FinishDodge();
	}
}

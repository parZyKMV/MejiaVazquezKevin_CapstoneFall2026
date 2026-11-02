#include "Magic/SpellComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatComponent.h"
#include "Combat/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/SpellData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Magic/ManaComponent.h"
#include "Magic/SpellProjectile.h"

USpellComponent::USpellComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpellComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
		{
			Health->OnHealthChanged.AddDynamic(this, &USpellComponent::HandleOwnerHealthChanged);
		}
	}
}

USpellComponent* USpellComponent::FindFromMesh(const USkeletalMeshComponent* MeshComp)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<USpellComponent>() : nullptr;
}

float USpellComponent::GetCooldownRemaining(int32 SlotIndex) const
{
	const UWorld* World = GetWorld();
	if (!World || !CooldownEnd.IsValidIndex(SlotIndex))
	{
		return 0.f;
	}
	return FMath::Max(0.f, CooldownEnd[SlotIndex] - World->GetTimeSeconds());
}

float USpellComponent::GetCooldownPercent(int32 SlotIndex) const
{
	if (!CooldownLength.IsValidIndex(SlotIndex) || CooldownLength[SlotIndex] <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(GetCooldownRemaining(SlotIndex) / CooldownLength[SlotIndex], 0.f, 1.f);
}

void USpellComponent::Fail(int32 SlotIndex, ESpellFailReason Reason)
{
	OnSpellFailed.Broadcast(SlotIndex, Reason);
}

bool USpellComponent::CastSpell(int32 SlotIndex)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	if (!EquippedSpells.IsValidIndex(SlotIndex) || !EquippedSpells[SlotIndex])
	{
		Fail(SlotIndex, ESpellFailReason::NoSpell);
		return false;
	}

	USpellData* Spell = EquippedSpells[SlotIndex];

	UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		Fail(SlotIndex, ESpellFailReason::Dead);
		return false;
	}

	// No se lanza mientras se esta lanzando o atacando con la espada.
	UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>();
	if (bCasting || (Combat && Combat->GetCombatState() == ECombatState::Attacking))
	{
		Fail(SlotIndex, ESpellFailReason::Busy);
		return false;
	}

	if (GetCooldownRemaining(SlotIndex) > 0.f)
	{
		Fail(SlotIndex, ESpellFailReason::OnCooldown);
		return false;
	}

	if (UManaComponent* Mana = Owner->FindComponentByClass<UManaComponent>())
	{
		if (!Mana->TrySpend(Spell->ManaCost))
		{
			Fail(SlotIndex, ESpellFailReason::NotEnoughMana);
			return false;
		}
	}

	// Recarga
	if (CooldownEnd.Num() < EquippedSpells.Num())
	{
		CooldownEnd.SetNumZeroed(EquippedSpells.Num());
		CooldownLength.SetNumZeroed(EquippedSpells.Num());
	}
	CooldownEnd[SlotIndex] = World->GetTimeSeconds() + Spell->Cooldown;
	CooldownLength[SlotIndex] = Spell->Cooldown;

	bCasting = true;
	bReleased = false;
	ActiveSlot = SlotIndex;
	++CastSerial;

	OnSpellCast.Broadcast(SlotIndex, Spell);

	// Animacion: el notify "Spell Release" suelta el hechizo. Sin montage, sale al instante.
	UAnimInstance* AnimInstance = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			AnimInstance = Mesh->GetAnimInstance();
		}
	}

	if (AnimInstance && Spell->CastMontage)
	{
		AnimInstance->Montage_Play(Spell->CastMontage, Spell->PlayRate);

		const int32 Serial = CastSerial;
		TWeakObjectPtr<USpellComponent> WeakThis(this);
		FOnMontageEnded EndDelegate;
		EndDelegate.BindLambda([WeakThis, Serial](UAnimMontage*, bool bInterrupted)
		{
			if (USpellComponent* Self = WeakThis.Get())
			{
				Self->HandleMontageEnded(Serial, bInterrupted);
			}
		});
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Spell->CastMontage);
	}
	else
	{
		ReleaseSpell();
		bCasting = false;
	}

	return true;
}

void USpellComponent::ReleaseSpell()
{
	if (!bCasting || bReleased || !EquippedSpells.IsValidIndex(ActiveSlot))
	{
		return;
	}

	bReleased = true;

	const USpellData* Spell = EquippedSpells[ActiveSlot];
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Spell || !Spell->ProjectileClass || !Owner || !World)
	{
		return;
	}

	// Origen: socket de la mano; si no existe, delante del pecho.
	FVector SpawnLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 80.f + FVector(0.f, 0.f, 40.f);
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (Spell->SpawnSocket != NAME_None && Mesh->DoesSocketExist(Spell->SpawnSocket))
			{
				SpawnLocation = Mesh->GetSocketLocation(Spell->SpawnSocket);
			}
		}
	}

	const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLocation);

	ASpellProjectile* Projectile = World->SpawnActorDeferred<ASpellProjectile>(
		Spell->ProjectileClass, SpawnTransform, Owner, Cast<APawn>(Owner),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Projectile)
	{
		Projectile->Damage = Spell->Damage;
		Projectile->StunDuration = Spell->StunDuration;
		Projectile->FinishSpawning(SpawnTransform);
	}
}

void USpellComponent::HandleMontageEnded(int32 Serial, bool bInterrupted)
{
	// Ignora finales de lanzamientos anteriores.
	if (Serial != CastSerial || !bCasting)
	{
		return;
	}

	// Si el montage termino sin notify, el hechizo sale igual (red de seguridad).
	if (!bReleased && !bInterrupted)
	{
		ReleaseSpell();
	}

	bCasting = false;
}

void USpellComponent::InterruptCast()
{
	if (!bCasting)
	{
		return;
	}

	bCasting = false;
	bReleased = true;
	++CastSerial; // invalida el final de montage pendiente

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (EquippedSpells.IsValidIndex(ActiveSlot) && EquippedSpells[ActiveSlot] && EquippedSpells[ActiveSlot]->CastMontage)
				{
					AnimInstance->Montage_Stop(0.15f, EquippedSpells[ActiveSlot]->CastMontage);
				}
			}
		}
	}
}

void USpellComponent::HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser)
{
	// Recibir dano a mitad de lanzamiento lo interrumpe.
	if (Delta < 0.f && bCasting)
	{
		InterruptCast();
	}
}

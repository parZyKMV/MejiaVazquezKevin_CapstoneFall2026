#include "Combat/CombatComponent.h"

#include "Combat/HealthComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Si el dueno tiene HealthComponent, dejamos de atacar cuando muere.
	if (AActor* Owner = GetOwner())
	{
		if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
		{
			Health->OnDeath.AddDynamic(this, &UCombatComponent::HandleOwnerDeath);
		}
	}
}

UCombatComponent* UCombatComponent::FindFromMesh(const USkeletalMeshComponent* MeshComp)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UCombatComponent>() : nullptr;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void UCombatComponent::RequestAttack()
{
	if (bOwnerDead || !ComboData)
	{
		return;
	}

	switch (State)
	{
	case ECombatState::Cooldown:
		return;

	case ECombatState::Idle:
		bAirCombo = IsOwnerFalling() && ComboData->AirAttacks.Num() > 0;
		BeginAttack(0);
		return;

	case ECombatState::Attacking:
		// Guardamos el clic: se usa cuando abra la ventana de combo (input buffer).
		bInputBuffered = true;
		TryAdvanceCombo();
		return;
	}
}

void UCombatComponent::InterruptCombo()
{
	if (State == ECombatState::Idle)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetAnimInstance())
	{
		if (UAnimMontage* Montage = CurrentMontage.Get())
		{
			CurrentMontage.Reset(); // asi el callback de fin de montage se ignora
			AnimInstance->Montage_Stop(0.15f, Montage);
		}
	}
	ResetCombo();
}

void UCombatComponent::HandleOwnerDeath(AActor* Victim, AActor* Killer)
{
	bOwnerDead = true;
	InterruptCombo();
}

// ---------------------------------------------------------------------------
// Combo
// ---------------------------------------------------------------------------

bool UCombatComponent::BeginAttack(int32 Index)
{
	UAnimInstance* AnimInstance = GetAnimInstance();
	const TArray<TObjectPtr<UAttackData>>& Attacks = GetCurrentAttacks();

	if (!AnimInstance || !Attacks.IsValidIndex(Index) || !Attacks[Index] || !Attacks[Index]->Montage)
	{
		ResetCombo();
		return false;
	}

	UAttackData* Attack = Attacks[Index];

	ComboIndex = Index;
	State = ECombatState::Attacking;
	bComboWindowOpen = false;
	bHitWindowActive = false;
	bHitConfirmed = false;
	bInputBuffered = false;
	HitActors.Reset();
	CurrentAttack = Attack;
	CurrentMontage = Attack->Montage; // antes de reproducir: el montage anterior termina y su callback se ignora

	const float Length = AnimInstance->Montage_Play(Attack->Montage, Attack->PlayRate);
	if (Length <= 0.f)
	{
		ResetCombo();
		return false;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UCombatComponent::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Attack->Montage);

	OnAttackStarted.Broadcast(Index, Attack);
	return true;
}

void UCombatComponent::TryAdvanceCombo()
{
	// Para avanzar: hay un clic guardado, la ventana de combo esta abierta y el ataque actual conecto.
	if (State != ECombatState::Attacking || !bComboWindowOpen || !bInputBuffered || !bHitConfirmed)
	{
		return;
	}

	const int32 Next = ComboIndex + 1;
	if (!GetCurrentAttacks().IsValidIndex(Next))
	{
		bInputBuffered = false; // ya era el ultimo ataque
		return;
	}

	BeginAttack(Next);
}

void UCombatComponent::ScheduleTryAdvance()
{
	// Se difiere un frame para no iniciar un montage dentro del callback de un notify.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UCombatComponent::TryAdvanceCombo);
	}
}

void UCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Si no es el montage actual, termino porque ya empezo el siguiente ataque: se ignora.
	if (Montage != CurrentMontage.Get() || State != ECombatState::Attacking)
	{
		return;
	}

	const bool bWasLastAttack = ComboIndex >= GetCurrentAttacks().Num() - 1;
	if (!bInterrupted && bWasLastAttack && bHitConfirmed)
	{
		BeginCooldown(); // combo completo
	}
	else
	{
		ResetCombo(); // fallo, no hubo clic a tiempo, o algo lo interrumpio
	}
}

void UCombatComponent::ResetCombo()
{
	ReleaseStunnedTargets();

	State = ECombatState::Idle;
	ComboIndex = 0;
	bComboWindowOpen = false;
	bHitWindowActive = false;
	bHitConfirmed = false;
	bInputBuffered = false;
	HitActors.Reset();
	CurrentAttack = nullptr;
	CurrentMontage.Reset();
}

void UCombatComponent::BeginCooldown()
{
	ReleaseStunnedTargets();

	State = ECombatState::Cooldown;
	ComboIndex = 0;
	bComboWindowOpen = false;
	bHitWindowActive = false;
	bHitConfirmed = false;
	bInputBuffered = false;
	HitActors.Reset();
	CurrentAttack = nullptr;
	CurrentMontage.Reset();

	const float Duration = ComboData ? FMath::Max(0.01f, ComboData->CooldownDuration) : 0.5f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CooldownTimer, this, &UCombatComponent::EndCooldown, Duration, false);
	}
	else
	{
		State = ECombatState::Idle;
	}
}

void UCombatComponent::EndCooldown()
{
	if (State == ECombatState::Cooldown)
	{
		State = ECombatState::Idle;
	}
}

void UCombatComponent::ReleaseStunnedTargets()
{
	for (const TWeakObjectPtr<UHealthComponent>& Target : StunnedTargets)
	{
		if (UHealthComponent* Health = Target.Get())
		{
			Health->ReleaseFromStun();
		}
	}
	StunnedTargets.Reset();
}

// ---------------------------------------------------------------------------
// Notifies (llamados desde los Anim Notify States)
// ---------------------------------------------------------------------------

void UCombatComponent::NotifyHitWindowBegin()
{
	if (State == ECombatState::Attacking)
	{
		bHitWindowActive = true;
	}
}

void UCombatComponent::NotifyHitWindowTick()
{
	if (State == ECombatState::Attacking && bHitWindowActive)
	{
		PerformHitTrace();
	}
}

void UCombatComponent::NotifyHitWindowEnd()
{
	bHitWindowActive = false;
}

void UCombatComponent::NotifyComboWindowBegin()
{
	if (State == ECombatState::Attacking)
	{
		bComboWindowOpen = true;
		ScheduleTryAdvance(); // por si el clic ya estaba guardado
	}
}

void UCombatComponent::NotifyComboWindowEnd()
{
	bComboWindowOpen = false;
	bInputBuffered = false;
}

// ---------------------------------------------------------------------------
// Deteccion de golpes
// ---------------------------------------------------------------------------

void UCombatComponent::PerformHitTrace()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !CurrentAttack)
	{
		return;
	}

	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Owner->GetActorForwardVector() * CurrentAttack->TraceRange;
	const float Radius = CurrentAttack->TraceRadius;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		FCollisionShape::MakeSphere(Radius),
		Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace)
	{
		DrawDebugLine(World, Start, End, FColor::Yellow, false, 0.1f);
		DrawDebugSphere(World, End, Radius, 12, Hits.Num() > 0 ? FColor::Green : FColor::Red, false, 0.1f);
	}
#endif

	AController* InstigatorController = nullptr;
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		InstigatorController = Pawn->GetController();
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || Target == Owner)
		{
			continue;
		}

		// Solo cuentan los actores que tienen vida (enemigos o props golpeables), no paredes ni piso.
		UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
		if (!Health || Health->IsDead())
		{
			continue;
		}

		// Cada objetivo se golpea una sola vez por ataque.
		const bool bAlreadyHit = HitActors.ContainsByPredicate(
			[Target](const TWeakObjectPtr<AActor>& Existing) { return Existing.Get() == Target; });
		if (bAlreadyHit)
		{
			continue;
		}
		HitActors.Add(Target);

		UGameplayStatics::ApplyDamage(Target, CurrentAttack->Damage, InstigatorController, Owner, UDamageType::StaticClass());
		Health->ApplyHitStun(CurrentAttack->StunDuration);

		const bool bAlreadyStunned = StunnedTargets.ContainsByPredicate(
			[Health](const TWeakObjectPtr<UHealthComponent>& Existing) { return Existing.Get() == Health; });
		if (!bAlreadyStunned)
		{
			StunnedTargets.Add(Health);
		}

		bHitConfirmed = true;
		OnAttackHit.Broadcast(Target, CurrentAttack);
	}

	if (bHitConfirmed)
	{
		ScheduleTryAdvance(); // por si el clic ya estaba guardado y la ventana ya esta abierta
	}
}

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------

const TArray<TObjectPtr<UAttackData>>& UCombatComponent::GetCurrentAttacks() const
{
	static const TArray<TObjectPtr<UAttackData>> Empty;
	if (!ComboData)
	{
		return Empty;
	}
	return bAirCombo ? ComboData->AirAttacks : ComboData->GroundAttacks;
}

UAnimInstance* UCombatComponent::GetAnimInstance() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			return Mesh->GetAnimInstance();
		}
	}
	return nullptr;
}

bool UCombatComponent::IsOwnerFalling() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			return Movement->IsFalling();
		}
	}
	return false;
}
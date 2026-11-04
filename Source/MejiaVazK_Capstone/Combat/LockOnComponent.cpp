#include "Combat/LockOnComponent.h"

#include "Combat/CombatComponent.h"
#include "Combat/HealthComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Combat/DodgeComponent.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		bDefaultUseControllerYaw = Character->bUseControllerRotationYaw;
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			bDefaultOrientToMovement = Movement->bOrientRotationToMovement;
		}
	}

	// Brazo de la camara: se guardan sus valores normales para devolverlos al soltar.
	if (AActor* Owner = GetOwner())
	{
		if (USpringArmComponent* Arm = Owner->FindComponentByClass<USpringArmComponent>())
		{
			CachedSpringArm = Arm;
			DefaultSocketOffset = Arm->SocketOffset;
			DefaultArmLength = Arm->TargetArmLength;
		}
	}

	SetComponentTickEnabled(false);
}

bool ULockOnComponent::UpdateCameraOffset(float DeltaTime, bool bLocked)
{
	USpringArmComponent* Arm = CachedSpringArm.Get();
	if (!Arm)
	{
		return true;
	}

	const FVector GoalOffset = bLocked ? LockCameraSocketOffset : DefaultSocketOffset;
	const float GoalLength = bLocked ? DefaultArmLength + LockCameraExtraDistance : DefaultArmLength;

	Arm->SocketOffset = FMath::VInterpTo(Arm->SocketOffset, GoalOffset, DeltaTime, CameraOffsetSpeed);
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, GoalLength, DeltaTime, CameraOffsetSpeed);

	return Arm->SocketOffset.Equals(GoalOffset, 0.5f) && FMath::IsNearlyEqual(Arm->TargetArmLength, GoalLength, 0.5f);
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ReleaseTarget();
	}
	else
	{
		AcquireTarget();
	}
}

bool ULockOnComponent::IsValidTarget(const AActor* Candidate) const
{
	if (!Candidate || Candidate == GetOwner())
	{
		return false;
	}

	const UHealthComponent* Health = Candidate->FindComponentByClass<UHealthComponent>();
	return Health && !Health->IsDead();
}

bool ULockOnComponent::HasLineOfSight(const AActor* Candidate) const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !Candidate || !World)
	{
		return false;
	}

	const FVector From = Owner->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector To = Candidate->GetActorLocation() + FVector(0.f, 0.f, AimHeightOffset);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnLineOfSight), false, Owner);
	Params.AddIgnoredActor(Candidate);

	FHitResult Hit;
	return !World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
}

bool ULockOnComponent::AcquireTarget()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	// Camara: de ahi sale el cono de busqueda.
	FVector CameraLocation = Owner->GetActorLocation();
	FRotator CameraRotation = Owner->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
		}
	}
	const FVector CameraForward = CameraRotation.Vector();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnSearch), false, Owner);
	World->OverlapMultiByObjectType(
		Overlaps, Owner->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SearchRadius), QueryParams);

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	TSet<AActor*> Seen;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Seen.Contains(Candidate))
		{
			continue;
		}
		Seen.Add(Candidate);

		if (!IsValidTarget(Candidate))
		{
			continue;
		}

		const FVector Aim = Candidate->GetActorLocation() + FVector(0.f, 0.f, AimHeightOffset);
		const FVector ToTarget = (Aim - CameraLocation).GetSafeNormal();
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(CameraForward, ToTarget), -1.f, 1.f)));
		if (AngleDeg > SearchHalfAngle)
		{
			continue;
		}

		if (bRequireLineOfSight && !HasLineOfSight(Candidate))
		{
			continue;
		}

		// Prefiere al que esta mas centrado en pantalla; entre parecidos, al mas cercano.
		const float Distance = FVector::Dist(Owner->GetActorLocation(), Candidate->GetActorLocation());
		const float Score = AngleDeg + Distance * 0.02f;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		return false;
	}

	Target = Best;
	SetComponentTickEnabled(true);
	OnTargetChanged.Broadcast(Best);
	return true;
}

void ULockOnComponent::ReleaseTarget()
{
	if (!Target.IsValid())
	{
		return;
	}

	Target.Reset();
	OnTargetChanged.Broadcast(nullptr);
	// El Tick sigue activo hasta restaurar el giro normal del personaje.
}

FVector ULockOnComponent::GetTargetAimLocation() const
{
	if (const AActor* T = Target.Get())
	{
		return T->GetActorLocation() + FVector(0.f, 0.f, AimHeightOffset);
	}

	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

void ULockOnComponent::SetLockedMovementMode(bool bLocked)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = bLocked ? false : bDefaultOrientToMovement;
	}
	Character->bUseControllerRotationYaw = bLocked ? true : bDefaultUseControllerYaw;

	bLockedModeApplied = bLocked;
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const UCombatComponent* Combat = Character->FindComponentByClass<UCombatComponent>();
	const UDodgeComponent* Dodge = Character->FindComponentByClass<UDodgeComponent>();
	const bool bAttacking = (Combat && Combat->GetCombatState() == ECombatState::Attacking) || (Dodge && Dodge->IsDodging());

	// Sin objetivo: devolver el giro normal (esperando a que termine un ataque en curso) y apagar el Tick.
	if (!Target.IsValid())
	{
		const bool bCameraSettled = UpdateCameraOffset(DeltaTime, false);

		if (bLockedModeApplied && !bAttacking)
		{
			SetLockedMovementMode(false);
		}
		if (!bLockedModeApplied && bCameraSettled)
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	AActor* LockedTarget = Target.Get();

	const UHealthComponent* OwnerHealth = Character->FindComponentByClass<UHealthComponent>();
	const bool bOwnerDead = OwnerHealth && OwnerHealth->IsDead();
	const float Distance = FVector::Dist(Character->GetActorLocation(), LockedTarget->GetActorLocation());

	if (bOwnerDead || !IsValidTarget(LockedTarget) || Distance > BreakDistance)
	{
		ReleaseTarget();
		return;
	}

	// Camara hacia el objetivo.
	if (AController* Controller = Character->GetController())
	{
		const FVector Eye = Character->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		FRotator Desired = (GetTargetAimLocation() - Eye).Rotation();
		Desired.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Desired.Pitch), MinPitch, MaxPitch);

		const FRotator NewRotation = FMath::RInterpTo(Controller->GetControlRotation(), Desired, DeltaTime, CameraTurnSpeed);
		Controller->SetControlRotation(NewRotation);
	}

	// Camara un poco mas alta y hacia un lado, para que el jugador no tape al enemigo.
	UpdateCameraOffset(DeltaTime, true);

	// El personaje mira al objetivo, salvo mientras ataca (lo controla CombatComponent).
	if (!bAttacking)
	{
		SetLockedMovementMode(true);
	}
}

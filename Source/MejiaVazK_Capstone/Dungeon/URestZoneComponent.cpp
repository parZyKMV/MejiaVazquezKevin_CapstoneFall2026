#include "Dungeon/URestZoneComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

TWeakObjectPtr<URestZoneComponent> URestZoneComponent::ActiveZone;

URestZoneComponent::URestZoneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// En el constructor NO se llama a SetBoxExtent ni a setters de colision:
	// crean el BodySetup con NewObject y eso crashea dentro de un constructor.
	InitBoxExtent(FVector(300.f, 300.f, 150.f));
	bHiddenInGame = true;
}

void URestZoneComponent::OnRegister()
{
	Super::OnRegister();

	// La colision se configura aqui (ya fuera del constructor). Siempre es la misma:
	// solo detecta al Pawn, sin bloquear nada.
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

void URestZoneComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &URestZoneComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &URestZoneComponent::HandleEndOverlap);
}

void URestZoneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(FadeOutTimer);
		TM.ClearTimer(WaitTimer);
		TM.ClearTimer(FadeInTimer);
	}

	if (ActiveZone.Get() == this)
	{
		ActiveZone.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

URestZoneComponent* URestZoneComponent::GetActiveRestZone()
{
	return ActiveZone.Get();
}

FTransform URestZoneComponent::GetWakeTransform() const
{
	const FTransform Comp = GetComponentTransform();
	return FTransform(Comp.GetRotation(), Comp.TransformPosition(WakeOffset));
}

void URestZoneComponent::SetPlayerInside(bool bInside)
{
	if (bPlayerInside != bInside)
	{
		bPlayerInside = bInside;
		OnPlayerInsideChanged.Broadcast(bInside);
	}
}

void URestZoneComponent::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
	{
		return;
	}

	CachedPlayer = Character;
	ActiveZone = this; // entrar aqui la convierte en el nuevo inicio / punto de reaparicion
	SetPlayerInside(true);
}

void URestZoneComponent::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (OtherActor && OtherActor == CachedPlayer.Get() && !bSequenceRunning)
	{
		SetPlayerInside(false);
	}
}

bool URestZoneComponent::TrySleep()
{
	ACharacter* Player = CachedPlayer.Get();
	if (!bPlayerInside || bSequenceRunning || !Player)
	{
		return false;
	}

	BeginSequence(Player, false);
	return true;
}

void URestZoneComponent::RespawnPlayerHere(ACharacter* Player)
{
	if (!Player || bSequenceRunning)
	{
		return;
	}

	BeginSequence(Player, true);
}

void URestZoneComponent::BeginSequence(ACharacter* Player, bool bTeleport)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bSequenceRunning = true;
	bDungeonReady = false;
	bTeleportOnFade = bTeleport;
	CachedPlayer = Player;
	ActiveZone = this;

	SetPlayerLocked(true);
	OnDoorChanged.Broadcast(false);
	Fade(0.f, 1.f, FadeOutTime);

	World->GetTimerManager().SetTimer(FadeOutTimer, this, &URestZoneComponent::OnFadedOut, FadeOutTime, false);
}

void URestZoneComponent::OnFadedOut()
{
	if (bTeleportOnFade)
	{
		if (ACharacter* Player = CachedPlayer.Get())
		{
			const FTransform Wake = GetWakeTransform();
			const FRotator WakeRot = Wake.GetRotation().Rotator();
			Player->TeleportTo(Wake.GetLocation(), WakeRot);
			if (AController* Controller = Player->GetController())
			{
				Controller->SetControlRotation(WakeRot);
			}
		}

		// Tras reaparecer el jugador esta dentro de la zona (permite dormir de inmediato).
		SetPlayerInside(true);
	}

	// Pantalla en negro: el generador destruye lo viejo y construye desde este cuarto.
	OnRegenerateRequested.Broadcast(GetOwner());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WaitTimer, this, &URestZoneComponent::OnRegenerateTimeout, MaxRegenerateWait, false);
	}
}

void URestZoneComponent::NotifyDungeonReady()
{
	UWorld* World = GetWorld();
	if (!World || !bSequenceRunning || bDungeonReady)
	{
		return;
	}

	bDungeonReady = true;
	World->GetTimerManager().ClearTimer(WaitTimer);
	World->GetTimerManager().SetTimer(FadeInTimer, this, &URestZoneComponent::BeginFadeIn, FMath::Max(BlackHoldTime, 0.01f), false);
}

void URestZoneComponent::OnRegenerateTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("RestZone %s: el generador no aviso a tiempo, se continua igual."), *GetNameSafe(GetOwner()));
	NotifyDungeonReady();
}

void URestZoneComponent::BeginFadeIn()
{
	OnDoorChanged.Broadcast(true);
	Fade(1.f, 0.f, FadeInTime);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FadeInTimer, this, &URestZoneComponent::FinishSequence, FadeInTime, false);
	}
}

void URestZoneComponent::FinishSequence()
{
	SetPlayerLocked(false);
	bSequenceRunning = false;
	bTeleportOnFade = false;
}

void URestZoneComponent::SetPlayerLocked(bool bLocked)
{
	if (ACharacter* Player = CachedPlayer.Get())
	{
		if (AController* Controller = Player->GetController())
		{
			Controller->SetIgnoreMoveInput(bLocked);
			Controller->SetIgnoreLookInput(bLocked);

			// Tambien se bloquean ataque, salto e interactuar mientras dura la secuencia.
			if (APlayerController* PC = Cast<APlayerController>(Controller))
			{
				if (bLocked)
				{
					Player->DisableInput(PC);
				}
				else
				{
					Player->EnableInput(PC);
				}
			}
		}
	}
}

void URestZoneComponent::Fade(float From, float To, float Duration)
{
	if (ACharacter* Player = CachedPlayer.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				// Al ir a negro (To > From) se mantiene el negro al terminar.
				PC->PlayerCameraManager->StartCameraFade(From, To, Duration, FLinearColor::Black, false, To > From);
			}
		}
	}
}
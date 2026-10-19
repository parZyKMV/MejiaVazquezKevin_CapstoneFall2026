#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "URestZoneComponent.generated.h"

class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRestRegenerateRequested, AActor*, RestRoomActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRestDoorChanged, bool, bOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRestPlayerInsideChanged, bool, bInside);

/**
 * Zona de descanso: un Box Collider que se agrega al Blueprint de un cuarto de descanso
 * (el que ya hereda de tu Master Dungeon Room). Tambien funciona como componente:
 * no hace falta que el cuarto cambie de clase padre.
 *
 * - Detecta que el jugador esta dentro del cuarto.
 * - Entrar la convierte en la zona ACTIVA (punto de reaparicion y nuevo inicio de la mazmorra).
 * - TrySleep(): fade a negro -> OnRegenerateRequested (el generador reconstruye) ->
 *   el generador llama NotifyDungeonReady() -> fade de vuelta.
 */
UCLASS(ClassGroup = (Dungeon), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API URestZoneComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	URestZoneComponent();

	/** Lo llama el input de Interactuar del jugador. True si empezo a dormir. */
	UFUNCTION(BlueprintCallable, Category = "RestZone")
	bool TrySleep();

	/** El generador llama esto al terminar de construir la mazmorra nueva. */
	UFUNCTION(BlueprintCallable, Category = "RestZone")
	void NotifyDungeonReady();

	/** Tras morir: fade, teleport al punto de despertar y regenerar la mazmorra. */
	UFUNCTION(BlueprintCallable, Category = "RestZone")
	void RespawnPlayerHere(ACharacter* Player);

	/** Ultima zona donde el jugador entro o durmio. Puede ser nula si aun no visito ninguna. */
	UFUNCTION(BlueprintPure, Category = "RestZone")
	static URestZoneComponent* GetActiveRestZone();

	UFUNCTION(BlueprintPure, Category = "RestZone")
	FTransform GetWakeTransform() const;

	UFUNCTION(BlueprintPure, Category = "RestZone")
	bool IsPlayerInside() const { return bPlayerInside; }

	UFUNCTION(BlueprintPure, Category = "RestZone")
	bool IsSequenceRunning() const { return bSequenceRunning; }

	/** El generador se engancha aqui al crear el cuarto de descanso. Recibe el actor del cuarto. */
	UPROPERTY(BlueprintAssignable, Category = "RestZone")
	FOnRestRegenerateRequested OnRegenerateRequested;

	/** Bind en el Blueprint del cuarto: cerrar (false) y abrir (true) la puerta. */
	UPROPERTY(BlueprintAssignable, Category = "RestZone")
	FOnRestDoorChanged OnDoorChanged;

	/** Para mostrar u ocultar el aviso "Dormir". */
	UPROPERTY(BlueprintAssignable, Category = "RestZone")
	FOnRestPlayerInsideChanged OnPlayerInsideChanged;

	/** Donde aparece el jugador, relativo a este box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RestZone")
	FVector WakeOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RestZone|Sleep", meta = (ClampMin = "0.1"))
	float FadeOutTime = 1.2f;

	/** Segundos en negro despues de que el generador termina (la "noche"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RestZone|Sleep", meta = (ClampMin = "0"))
	float BlackHoldTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RestZone|Sleep", meta = (ClampMin = "0.1"))
	float FadeInTime = 1.5f;

	/** Si el generador no avisa en este tiempo se continua igual (evita negro eterno). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RestZone|Sleep", meta = (ClampMin = "1"))
	float MaxRegenerateWait = 8.f;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void BeginSequence(ACharacter* Player, bool bTeleport);
	void OnFadedOut();
	void OnRegenerateTimeout();
	void BeginFadeIn();
	void FinishSequence();

	void SetPlayerInside(bool bInside);
	void SetPlayerLocked(bool bLocked);
	void Fade(float From, float To, float Duration);

	bool bPlayerInside = false;
	bool bSequenceRunning = false;
	bool bDungeonReady = false;
	bool bTeleportOnFade = false;

	TWeakObjectPtr<ACharacter> CachedPlayer;

	FTimerHandle FadeOutTimer;
	FTimerHandle WaitTimer;
	FTimerHandle FadeInTimer;

	static TWeakObjectPtr<URestZoneComponent> ActiveZone;
};
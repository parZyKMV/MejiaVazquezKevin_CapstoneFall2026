#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "DodgeComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UHealthComponent;

UENUM(BlueprintType)
enum class EDodgeFailReason : uint8
{
	NotEnoughStamina UMETA(DisplayName = "Not Enough Stamina"),
	Busy UMETA(DisplayName = "Busy"),
	OnCooldown UMETA(DisplayName = "On Cooldown"),
	InAir UMETA(DisplayName = "In Air"),
	Dead UMETA(DisplayName = "Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeStartedSignature, FVector, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeFailedSignature, EDodgeFailReason, Reason);

/**
 * Esquive (roll) que gasta stamina.
 *
 * Dodge(): valida (vivo, no atacando ni lanzando, en el suelo, sin cooldown, con stamina),
 * gira al personaje hacia donde se mueve (sin direccion: hacia adelante), reproduce el montage
 * (root motion, igual que los ataques) y da invulnerabilidad durante IFrameDuration segundos.
 * Si no hay montage usa un impulso corto (solo para pruebas).
 *
 * Requiere UHealthComponent::bInvulnerable.
 * Durante el esquive el input de ataque y de hechizos debe ignorarse (IsDodging, en Blueprint).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDodgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float StaminaCost = 25.f;

	/** Segundos de espera tras terminar un esquive antes de poder repetirlo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float Cooldown = 0.2f;

	/** Segundos, desde que empieza, en que no se recibe dano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float IFrameDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	bool bAllowInAir = false;

	/** Animacion de esquive con root motion (slot DefaultSlot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animation", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	/** Sin montage: velocidad del impulso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Fallback", meta = (ClampMin = "0"))
	float FallbackDashSpeed = 1100.f;

	/** Sin montage: duracion del esquive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Fallback", meta = (ClampMin = "0.05"))
	float FallbackDuration = 0.3f;

	UPROPERTY(BlueprintAssignable, Category = "Dodge|Events")
	FOnDodgeStartedSignature OnDodgeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dodge|Events")
	FOnDodgeEndedSignature OnDodgeEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dodge|Events")
	FOnDodgeFailedSignature OnDodgeFailed;

	/** Intenta esquivar. Devuelve true si empezo. */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	bool Dodge();

	UFUNCTION(BlueprintPure, Category = "Dodge")
	bool IsDodging() const { return bDodging; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	FVector GetDodgeDirection(const ACharacter* Character) const;
	void SetInvulnerable(bool bValue);
	void EndInvulnerability();
	void FinishDodge();
	void HandleMontageEnded(int32 Serial);
	void Fail(EDodgeFailReason Reason);

	TWeakObjectPtr<UHealthComponent> CachedHealth;

	FTimerHandle IFrameTimer;
	FTimerHandle FallbackTimer;

	bool bDodging = false;
	int32 DodgeSerial = 0;
	float NextDodgeTime = 0.f;

	/** Giro del personaje antes del esquive, para devolverlo al terminar. */
	bool bSavedUseControllerYaw = false;
	bool bSavedOrientToMovement = true;
};

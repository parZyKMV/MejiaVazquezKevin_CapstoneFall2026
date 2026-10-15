#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AttackData.h"
#include "Data/ComboData.h"
#include "CombatComponent.generated.h"

class UHealthComponent;
class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle,
	Attacking,
	Cooldown
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttackStartedSignature, int32, ComboIndex, UAttackData*, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttackHitSignature, AActor*, Target, UAttackData*, Attack);

/**
 * Combo de un boton (estilo Kingdom Hearts):
 *  - El primer clic inicia el combo (suelo o aire segun el personaje).
 *  - Solo si el ataque CONECTA, el siguiente clic (dentro de la ventana de combo) avanza al siguiente ataque.
 *  - Si el ataque falla o no hay clic a tiempo, el combo se reinicia.
 *  - Despues del ultimo golpe hay un cooldown.
 *  - Cada golpe que conecta aturde al enemigo unos segundos (UHealthComponent::ApplyHitStun).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	/** Combo que usa este personaje. Asignalo en el Blueprint del personaje. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UComboData> ComboData;

	/** Dibuja el trace de golpe en pantalla para ajustar alcance y radio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAttackStartedSignature OnAttackStarted;

	/** Se lanza por cada enemigo o prop golpeado (util para sonido, hitstop, efectos). */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAttackHitSignature OnAttackHit;

	/** Llamar desde el input de ataque (por ejemplo, clic izquierdo). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RequestAttack();

	/** Corta el combo de inmediato (por ejemplo, si el jugador recibe dano; decision de diseno pendiente). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void InterruptCombo();

	UFUNCTION(BlueprintPure, Category = "Combat")
	ECombatState GetCombatState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetComboIndex() const { return ComboIndex; }

	// --- Los llaman los Anim Notify States del montage ---
	void NotifyHitWindowBegin();
	void NotifyHitWindowTick();
	void NotifyHitWindowEnd();
	void NotifyComboWindowBegin();
	void NotifyComboWindowEnd();

	/** Busca el CombatComponent del dueno de una malla (para los notifies). */
	static UCombatComponent* FindFromMesh(const USkeletalMeshComponent* MeshComp);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerDeath(AActor* Victim, AActor* Killer);

	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool BeginAttack(int32 Index);
	void TryAdvanceCombo();
	void ScheduleTryAdvance();
	void PerformHitTrace();
	void ResetCombo();
	void BeginCooldown();
	void EndCooldown();
	void ReleaseStunnedTargets();
	void BeginAirHover();
	void EndAirHover();

	const TArray<TObjectPtr<UAttackData>>& GetCurrentAttacks() const;
	UAnimInstance* GetAnimInstance() const;
	bool IsOwnerFalling() const;

	ECombatState State = ECombatState::Idle;
	int32 ComboIndex = 0;
	bool bAirCombo = false;
	bool bOwnerDead = false;

	bool bComboWindowOpen = false;
	bool bHitWindowActive = false;
	bool bHitConfirmed = false;
	bool bInputBuffered = false;

	bool bHovering = false;
	float SavedGravityScale = 1.f;

	UPROPERTY()
	TObjectPtr<UAttackData> CurrentAttack;

	TWeakObjectPtr<UAnimMontage> CurrentMontage;
	TArray<TWeakObjectPtr<AActor>> HitActors;
	TArray<TWeakObjectPtr<UHealthComponent>> StunnedTargets;

	FTimerHandle CooldownTimer;
};
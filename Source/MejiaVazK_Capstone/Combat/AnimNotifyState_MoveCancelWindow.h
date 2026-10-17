#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MoveCancelWindow.generated.h"

/**
 * Marca en el montage el final del ataque en el que el jugador recupera el control.
 * Mientras esta activo, el personaje puede moverse y rotar; si se mueve, el ataque se cancela.
 * Colocalo en el tramo de recuperacion (el final de la animacion).
 */
UCLASS()
class MEJIAVAZK_CAPSTONE_API UAnimNotifyState_MoveCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
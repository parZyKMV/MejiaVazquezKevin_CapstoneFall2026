#include "Combat/AnimNotifyState_MoveCancelWindow.h"

#include "Combat/CombatComponent.h"

void UAnimNotifyState_MoveCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyMoveCancelBegin();
	}
}

void UAnimNotifyState_MoveCancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyMoveCancelTick();
	}
}

void UAnimNotifyState_MoveCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyMoveCancelEnd();
	}
}

FString UAnimNotifyState_MoveCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("Move Cancel");
}